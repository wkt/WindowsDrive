#include "qsiapplication.h"

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QLockFile>

static const char *QSI_NAME_PREFIX = "__qsi__.";
static const char *ARGS_END = "&&  \n";

inline static QString get_current_username()
{
    QString name = qgetenv("USER");
    if (name.isEmpty())
        name = qgetenv("USERNAME");
    return name;
}

class QSIApplication::Impl{

    Impl():server(NULL),qlf(NULL){

    }

    ~Impl(){
        if(qlf){
            if(qlf->isLocked())qlf->unlock();
            delete qlf;
            qlf = NULL;
        }
        if(server!=NULL)delete server;
        server = NULL;
    }

    friend class QSIApplication;
private:
    QLocalServer *server;
    QLockFile *qlf;

protected:

    inline QString serverName(const QApplication &app){
        QString app_name = app.applicationName();
        QString org_name = app.organizationName();
        if(app_name.isEmpty()){
            throw "Application name is empty";
        }
        if(org_name.isEmpty()){
            throw "Organization name is empty";
        }
        QString sn =  QSI_NAME_PREFIX + get_current_username()+"."+app_name+"."+org_name;
        return sn;
    }

    inline bool connectServer(const QApplication &app){
        QLocalSocket socket;
        socket.connectToServer(serverName(app));
        bool ret = false;
        if(socket.waitForConnected(1000)){
            QStringList args = app.arguments();
            //qDebug()<<"args: "<<args;
            for(int i=0;i<args.size();i++){
                socket.write((args[i]+"\n").toUtf8());
                socket.flush();
                socket.waitForBytesWritten(500);
            }
            socket.write(ARGS_END);
            socket.flush();
            socket.readAll();
            ret = true;
        }
        socket.close();
        return ret;
    }

    inline bool setupServer(const QApplication& app){
        server = new QLocalServer();
        server->setSocketOptions(QLocalServer::WorldAccessOption);
        QString lfn=lockFilename(app);
        //qDebug()<<"lfn:"<<lfn<<"\n";
        qlf = new QLockFile(lfn);
        if(!qlf->tryLock()){
            return false;
         }
        return server->listen(serverName(app));
    }

    inline const QString lockFilename(const QApplication& app){
        QString lockd = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(lockd);
        return lockd+ QDir::separator()+serverName(app)+".lock";
    }
};

QSIApplication::QSIApplication(int &argc, char **argv, int flags)
    :QApplication(argc,argv,flags)
    ,impl(new QSIApplication::Impl)
{

}

QSIApplication::~QSIApplication()
{
    delete impl;
}

int QSIApplication::checkInstance()
{
    int r = processArguments();

    if(impl->setupServer(*this)){
         QObject::connect(impl->server,&QLocalServer::newConnection,this,&QSIApplication::handleNewArguments);
    }else{
        impl->connectServer(*this);
        r = 1;
    }
    return r;
}

int QSIApplication::processArguments(const bool& allow_unknown_option)
{
    QCommandLineParser parser;
    handleAddOptions(parser);
    if(allow_unknown_option){
        parser.parse(this->arguments());
    }else{
        parser.process(this->arguments());
    }
    return handleArguments(parser);
}

void QSIApplication::handleNewArguments()
{
    if(impl->server == nullptr)return;
    QLocalSocket *socket = impl->server->nextPendingConnection();
    //qDebug()<<__PRETTY_FUNCTION__<<",socket:"<<socket;
    QStringList args;
    QByteArray array;
    QString s;
    while(true){
        socket->waitForReadyRead(100);
        array = socket->readLine();
        if(array.size() <= 0){
            break;
        }
        s = QString::fromUtf8(array);
        if(s == ARGS_END){
            break;
        }
        s = s.remove(s.length()-1,1);
        args.append(s);
    }
    socket->write("\n");
    socket->flush();
    socket->close();
    QCommandLineParser parser;
    handleAddOptions(parser);
    parser.parse(args);
    handleArguments(parser);
}

