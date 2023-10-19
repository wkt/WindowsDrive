#include "networkmanager.h"

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QBasicTimer>
#include <QTimerEvent>
#include "config.h"
#include "handlereply.h"
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QByteArray>
#include <cstdlib>
#include <ctime>
#include "config.h"
#include "utils.h"

const char *KEY_VER = "ver";
const char *KEY_DATE = "date";
const char *KEY_LOG = "log";
const char *KEY_URL = "url";

inline static QString client_id_file()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    std::string out;
    command_read("mkdir -p '"+dir.toStdString()+"'",out);
    return dir + QDir::separator()+".client_id";
}

inline static QString
hash(const QByteArray& data,const QCryptographicHash::Algorithm method=QCryptographicHash::Algorithm::Sha256)
{
    QCryptographicHash h(method);
    h.addData(data);
    return h.result().toHex();
}

inline static void random_bytes(void *_s,size_t len)
{
    srand(time(NULL));
    unsigned char *s = static_cast<unsigned char*>(_s);
    for(size_t i=0;i<len;i++){
        int n = rand();
        unsigned char c = static_cast<char>(256.*(1.0*n/RAND_MAX));
        if(c<=0)c=1;
        else if(c >= 255)c = 254;
        s[i]=c;
    }
}

inline static const QString
get_client_id()
{
    static char buf[257] = {0};
    if(buf[0] == 0){
        qint64 n = 0;
        QString f1 = "/etc/machine-id";
        if(QFile::exists(f1)){
            QFile file(f1);
            file.open(QIODevice::ReadOnly);
            n = file.read(buf,sizeof(buf)-1);
        }
        if(n == 0){
            QString f2 = client_id_file();
            if(QFile::exists(f2)){
                QFile file(f2);
                file.open(QIODevice::ReadOnly);
                n = file.read(buf,sizeof(buf)-1);
            }
            if(n == 0){
                random_bytes(buf,sizeof(buf)-1);
                QFile file(f2);
                file.open(QIODevice::WriteOnly);
                file.write(buf,sizeof(buf));
                file.flush();
            }
        }
    }
    return hash(buf);
}


class QReplyTimeout : public QObject {

    enum HandleMethod { Abort, Close };

    QBasicTimer m_timer;
    HandleMethod m_method;

public:
    QReplyTimeout(QNetworkReply* reply, const int timeout_msec, HandleMethod method = Abort):QObject(reply), m_method(method)
    {
        Q_ASSERT(reply);
        if (reply && reply->isRunning()) {
          m_timer.start(timeout_msec, this);
          connect(reply, &QNetworkReply::finished, this, &QObject::deleteLater);
        }else{
            deleteLater();
        }
    }

    static void set(QNetworkReply* reply, const int timeout_msec, HandleMethod method = Abort) {
        new QReplyTimeout(reply, timeout_msec, method);
    }

protected:
  void timerEvent(QTimerEvent * ev)
  {
    if (!m_timer.isActive() || ev->timerId() != m_timer.timerId())
      return;
    auto reply = static_cast<QNetworkReply*>(parent());
    if (reply->isRunning()) {
      if (m_method == Close){
        reply->close();
      }else{
        reply->abort();
      }
      m_timer.stop();
    }
  }
};

NetworkManager::NetworkManager(QObject *parent)
    : QObject{parent},nm{new QNetworkAccessManager}
{
    if(!QSslSocket::supportsSsl()){
        qWarning()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
                <<"sslLibraryBuildVersionString:"<<QSslSocket::sslLibraryBuildVersionString()
                <<", "
#endif
                <<"sslLibraryVersionString: "<<QSslSocket::sslLibraryVersionString();
    }
}

NetworkManager::~NetworkManager()
{
    delete nm;
}


void NetworkManager::checkUpdate(QObject *receiver, ReceiverFunc func)
{
    if(strlen(HTTP_API_URL)<8){
        return;
    }

    QString url = HTTP_API_URL + QString("/check_update");
    QNetworkRequest requestInfo((QUrl(url)));
    requestInfo.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery query;

    QString osName;
    QString osVer;
    QString kerName;
    QString kerVer;
    QString cpuArch;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    osName = QSysInfo::productType();
    osVer = QSysInfo::productVersion();
    kerName = QSysInfo::kernelType();
    kerVer = QSysInfo::kernelVersion();
    cpuArch = QSysInfo::currentCpuArchitecture();
#else
    osName = sys_product_type().data();
    osVer = sys_product_ver().data();
    kerName = sys_kernel_type().data();
    kerVer = sys_kernel_ver().data();
    cpuArch = sys_cpu_arch().data();
#endif

    query.addQueryItem("os",osName);
    query.addQueryItem("osVer",osVer);
    query.addQueryItem("kernel",kerName);
    query.addQueryItem("kernelVer",kerVer);
    query.addQueryItem("cpuArch",cpuArch);
    query.addQueryItem("clientId",get_client_id());

    query.addQueryItem("appVer",VERSION);

    query.addQueryItem("appName",APPLICATION_NAME);
    query.addQueryItem("appimage",is_appimage_executable()?"1":"0");
    query.addQueryItem("lang",QLocale::system().uiLanguages().join(","));

    QNetworkReply *reply = nm->post(requestInfo,query.toString().toUtf8());
    new HandleReply(reply,(QObject*)receiver,(ReceiverFunc)func);
    QReplyTimeout::set(reply,3000);
}
