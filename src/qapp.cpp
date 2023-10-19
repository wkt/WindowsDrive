#include "qapp.h"
#include "mainwindow.h"
#include "passwddialog.h"
#include "ntmounteddevices.h"
#include "settingsdialog.h"
#include "networkmanager.h"

#include <QSystemTrayIcon>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QSettings>
#include <QMenu>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QTextCodec>
#include <QDesktopServices>
#include <QUrl>
#include <QStyleFactory>
#include <QString>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QList>
#include <QTimer>
#include <time.h>
#include "functhread.h"
#include "utils.h"
#include "config.h"

#include <sys/utsname.h>
#include <stdio.h>

static const int ACTION_SETTINGS = 1;
static const int ACTION_MAIN_WINDOW = 2;
static const int ACTION_ABOUT = 3;
static const int ACTION_RELOAD = 4;
static const int ACTION_QUIT = -1;

#define SET_VALUE(K,V) impl->settings.setValue((K),QVariant::fromValue(V))
#define GET_VALUE(K,D) impl->settings.value((K),QVariant::fromValue(D))

inline static const QString autostart_desktop_file()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+ QDir::separator() +"autostart";
    command_run("mkdir -p \""+dir.toStdString()+"\"");
    return dir + QDir::separator() + QString(APPLICATION_NAME).toLower() +".desktop";
}

inline static const QString locale_hicolor_icon_file()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/hicolor/scalable/apps";
    command_run("mkdir -p \""+dir.toStdString()+"\"");
    return dir + QDir::separator() + "windows-drive.svg";
}

inline static bool isActiveModalWidgetSame(const QString &className)
{
    QWidget *amw = QApplication::activeModalWidget();
    if(amw == nullptr)return false;
    if(amw->metaObject()->className() == className){
        return true;
    }
    return false;
}

inline static bool isActiveModalWidgetSame(const QWidget* widget)
{
    if(widget == nullptr)return false;
    return isActiveModalWidgetSame(widget->metaObject()->className());
}

class QApp::QAppImpl
{
protected:
    MainWindow *mainWin;
    QSystemTrayIcon *trayIcon;
    NTMountedDevices ntmd;
    QMutex mutex;
    QSettings settings;
    QMenu  *trayMenu;
    QMenu  *driveMenu;
    bool   _is_exec;
    QAction *reloadAction;
    PasswdDialog *pdlg;
    bool autostart;
    NetworkManager *nm;

    friend class QApp;

    QAppImpl():mainWin(NULL),
        trayIcon(new QSystemTrayIcon),
        trayMenu(new QMenu),_is_exec(false),
        reloadAction(NULL),
        pdlg(NULL),autostart(false),
        nm(NULL)
    {
    }

    ~QAppImpl(){
        reloadAction = NULL;
        delete trayMenu;
        delete trayIcon;
    }

};


QApp::QApp(int &argc, char **argv):
    QSIApplication(argc,argv),
    impl(new QAppImpl)
{
    setStyle(QStyleFactory::create("Fusion"));
    //setApplicationDisplayName(tr(APPLICATION_NAME));

    //qDebug()<<__FUNCTION__<<",settings: "<<impl->settings.fileName();
}

QApp::~QApp()
{
    impl->settings.sync();
    if(impl->mainWin){
        delete impl->mainWin;
        impl->mainWin = NULL;
    }
    delete impl;
}

int QApp::exec()
{

    add_executable_search_paths();

    impl->mainWin = new MainWindow();
    QAction *a;
    a = impl->trayMenu->addAction(tr("Main Window"));
    a->setData(QVariant::fromValue(ACTION_MAIN_WINDOW));

    a = impl->trayMenu->addAction(tr("Settings"));
    a->setData(QVariant::fromValue(ACTION_SETTINGS));

    impl->trayMenu->addSeparator();

    impl->driveMenu = impl->trayMenu->addMenu(tr("Windows and Drive(s)"));

    a = impl->trayMenu->addAction(tr("Reload"));
    a->setData(QVariant::fromValue(ACTION_RELOAD));
    impl->reloadAction = a;

    impl->mainWin->addAction(a);

    impl->trayMenu->addSeparator();

    a = impl->trayMenu->addAction(tr("About"));
    a->setData(QVariant::fromValue(ACTION_ABOUT));

    a = impl->trayMenu->addAction(tr("Quit"));
    a->setData(QVariant::fromValue(ACTION_QUIT));

    impl->trayIcon->setIcon(QIcon(":/icons/app.svg"));
    impl->trayIcon->setContextMenu(impl->trayMenu);

    connect(impl->trayMenu,&QMenu::triggered,this,&QApp::handleAction);
    connect(impl->driveMenu,&QMenu::triggered,this,&QApp::handleAction);

    if(!impl->autostart){
        impl->mainWin->show();
    }
    impl->mainWin->center();
    impl->trayIcon->show();
    loadWindowsDrive();

    impl->_is_exec = true;
    if(isCheckUpdateOnStart()){
        checkUpdate();
    }
    return QApplication::exec();
}

void QApp::load_window_drive_thread()
{
    int64_t t = get_time_ms();
    impl->mutex.lock();
    impl->ntmd.scan_paritions();
    if(canMountWindowsPartition())
        impl->ntmd.mount_paritions();

    impl->ntmd.load_windows();
    impl->ntmd.set_ntregf(selectedWindowDevice().toStdString());
    t = get_time_ms()-t;
    long delay_ms = 200;
    if(t<delay_ms){
        QThread::msleep(MAX(delay_ms/2,delay_ms-t));
    }
    impl->mutex.unlock();
}

static void (QApp::*__qapp_fun)() = NULL;

inline static void __load_window_drive_thread(void *data,void* arg1)
{
    QApp* app = static_cast<QApp*>(data);
    if(__qapp_fun == NULL)return;
    (app->*__qapp_fun)();
}

void QApp::loadWindowsDrive()
{
    if(!authenticationRequried()){
        handleLoadWindowsDriveFinished();
        return;
    }
    impl->reloadAction->setEnabled(false);
    impl->driveMenu->setEnabled(false);
    if(impl->mainWin->isVisible() && !impl->mainWin->isHidden()) impl->mainWin->showLoading(tr("Loading ..."));
    if(__qapp_fun == NULL)__qapp_fun = &QApp::load_window_drive_thread;
    QThread *thread = new FuncThread(__load_window_drive_thread,static_cast<void*>(this));
    QObject::connect(thread,&QThread::finished,thread,&QObject::deleteLater);
    QObject::connect(thread,&QThread::finished,this,&QApp::handleLoadWindowsDriveFinished);
    thread->start();
}

bool QApp::canMountWindowsPartition()
{
    return impl->settings.value(KEY_MOUNT_WINDOW_PARTITION,QVariant::fromValue(true)).toBool();
}

void QApp::setMountWindowsPartition(bool enabled)
{
    impl->settings.setValue(KEY_MOUNT_WINDOW_PARTITION,QVariant::fromValue(enabled));
}

const QString QApp::selectedWindowDevice()
{
    return GET_VALUE(KEY_SELECTED_WINDOW_DEV_PATH,QString("")).toString();
}

void QApp::setSelectedWindowDevice(const QString& dev_path)
{
    SET_VALUE(KEY_SELECTED_WINDOW_DEV_PATH,dev_path);
    impl->ntmd.set_ntregf(dev_path.toStdString());
    updateWindowsDriveUI();
}

const std::vector<NTRegf>& QApp::get_windows()
{
    return impl->ntmd.get_windows();
}

void QApp::handleLoadWindowsDriveFinished()
{
    impl->reloadAction->setEnabled(true);
    impl->mainWin->hideLoading();


    const auto & nt_list = impl->ntmd.get_windows();
    impl->mainWin->setWindowsList(&nt_list);

    updateWindowsDriveUI();

}



void QApp::showAbout()
{
    if(isActiveModalWidgetSame("QMessageBox")){
        showMainWindow();
        return;
    }
    QString version = VERSION;

    QString home_url(HOME_URL);
    home_url = home_url.append("?lang="+QLocale::system().uiLanguages().join(","));

    char aboutText[1025] = {0};
    QString pt;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    pt = QSysInfo::productType();
#else
    pt = sys_product_type().data();
#endif

    QString fmt=tr(
                "<center>"
                "<h3>WindowsDrive  %s</h3>"
                "<p>WindowsDrive is a helper for <b>%s</b> user to show Windows(if installed) drive letter for local disk(s).</p>"
                "<p><a href=\"%s\">Official Website</a></p>"
                "<p>%s</p>"
                "</center><br/>"
                );
    snprintf(aboutText,sizeof(aboutText),
                fmt.toStdString().data(),
                version.toStdString().data(),
                pt.toUtf8().data(),
                home_url.toStdString().data(),
                tr(COPY_RIGHT).toStdString().data());
    impl->mainWin->show();
    QMessageBox::about(impl->mainWin, tr("About"), aboutText);
}

void QApp::showMainWindow()
{
    impl->mainWin->showNormal();
    impl->mainWin->activateWindow();
    impl->mainWin->raise();
}

void QApp::handleQuit()
{
    impl->settings.sync();
    this->quit();
}

void QApp::showSettingsDialog()
{
    if(isActiveModalWidgetSame("SettingsDialog")){
        showMainWindow();
        return;
    }
   SettingsDialog dlg(impl->mainWin);
   dlg.exec();
   dlg.hide();
}

bool QApp::canAutostart()
{
    return GET_VALUE(KEY_AUTOSTART,false).toBool() && QFileInfo(autostart_desktop_file()).isFile();
}

void QApp::setAutostart(const bool& enabled)
{
    const QString dkp = autostart_desktop_file();
    if(enabled){
        QFile::copy(":/autostart.desktop",dkp);
        command_run("chmod ug+rw '"+dkp.toStdString()+"';");
        if(is_appimage_executable()){
            const QString icons_f = locale_hicolor_icon_file();
            QFile::copy(":/icons/app.svg",icons_f);
            std::string cmd = "sed -i 's|Exec=WindowsDrive|Exec=\"" + get_appimage_path() +"\"|g' "+dkp.toStdString();
            command_run(cmd);
        }
    }else{
        QFile(dkp).remove();
    }
    SET_VALUE(KEY_AUTOSTART,enabled);
}

void QApp::handleAction(QAction *action)
{
    const QVariant &v = action->data();
    switch(v.toInt()){

    case ACTION_SETTINGS:
        showSettingsDialog();
        break;
    case ACTION_MAIN_WINDOW:
        showMainWindow();
        break;
    case ACTION_ABOUT:
        showAbout();
        break;
    case ACTION_QUIT:
        handleQuit();
        break;
    case ACTION_RELOAD:
        showMainWindow();
        loadWindowsDrive();
        break;
    default:
        ;
    }
    const QString& vs = v.toString();
    if(vs[0] == '/'){
        handleOpenDirectory(vs);
    }
}

void QApp::handleOpenDirectory(const QString &dirpath)
{
    QDesktopServices::openUrl(QUrl("file://"+dirpath));
}

bool QApp::isHideOnClose()
{
    return GET_VALUE(KEY_HIDE_ON_CLOSE,true).toBool();
}

void QApp::setHideOnClose(bool enabled)
{
    SET_VALUE(KEY_HIDE_ON_CLOSE,enabled);
}

void QApp::setCheckUpdateOnStart(const bool& enabled)
{
    SET_VALUE(KEY_ON_START_CHECK_UPDATE,enabled);
}

bool QApp::isCheckUpdateOnStart()
{
    return GET_VALUE(KEY_ON_START_CHECK_UPDATE,true).toBool();
}


void QApp::handleAddOptions(QCommandLineParser &parser)
{
    parser.setApplicationDescription(tr(""));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringList("quit")<<"q",""));
    parser.addOption(QCommandLineOption("autostart",""));

}

int QApp::handleArguments(const QCommandLineParser &parser)
{

    if(parser.isSet("q") || parser.isSet("quit")){
        if(impl->_is_exec)handleQuit();
        return 2;
    }

    if(parser.isSet("autostart")){
        impl->autostart = true;
        return 0;
    }

    if(impl->_is_exec)showMainWindow();
    return 0;
}

static void (QApp::*qapp_verify_func)(const QString*) = NULL;

inline static void __qapp_auth_verify(void *obj0,void *obj1)
{
    QApp *app=static_cast<QApp*>(obj0);
    QString *psw = static_cast<QString*>(obj1);
    if(qapp_verify_func == NULL)return;
    (app->*qapp_verify_func)(psw);
}

bool QApp::authenticationRequried()
{

    if(!is_need_su()){
        return true;
    }

    if(can_setuid(WD_HELPER)){
        return true;
    }

    bool ret = false;
    impl->pdlg = new PasswdDialog(impl->mainWin);
    //impl->pdlg->setWindowFlags(Qt::Drawer| Qt::Sheet);
    impl->pdlg->setWindowModality(Qt::ApplicationModal);
    impl->pdlg->setModal(true);
    int r = QDialog::Rejected;
    QString *psw = new QString();
    while(true){
        impl->pdlg->show();
        r = impl->pdlg->exec();
        if(r == -1){
            ret = true;
            break;
        }
        if(r == -2){
            impl->pdlg->setEnabled(true);
            continue;
        }
        if(r == QDialog::Rejected)break;
        *psw = impl->pdlg->password();
        //if(psw->isEmpty())continue;
        impl->pdlg->setEnabled(false);
        if(qapp_verify_func == NULL)qapp_verify_func = &QApp::authentication_verify;
        QThread *thread = new FuncThread(__qapp_auth_verify,this,psw);
        QObject::connect(thread,&QThread::finished,thread,&QObject::deleteLater);
        thread->start();
    }
    delete psw;
    impl->pdlg->hide();
    delete impl->pdlg;
    impl->pdlg = NULL;
    return ret;
}



void QApp::authentication_verify(const QString* psw)
{
    QString passwd = *psw;
    int _done = 0;
    int delay_ms=1000;
    if(is_sudo_password_correct(passwd.toStdString())){
        bool bl = setup_setuid(WD_HELPER,passwd.toStdString());
        //std::cerr<<"setup_setuid: "<<bl<<std::endl;
        if(bl)delay_ms=600;
        _done = -1;
    }else{
        _done = -2;
    }
    //make a delay
    QThread::msleep(delay_ms);
    impl->pdlg->done(_done);
}

static void main_window_showUpdateInfo(QObject *obj,const void *res)
{
    MainWindow *mw = static_cast<MainWindow*>(obj);
    mw->showUpdateInfo((const QJsonObject*)res);
}

void QApp::checkUpdate()
{
    if(impl->nm == NULL){
        impl->nm = new NetworkManager;
    }
    impl->nm->checkUpdate(static_cast<QObject*>(impl->mainWin),static_cast<ReceiverFunc>(&main_window_showUpdateInfo));
}

void QApp::updateWindowsDriveUI()
{
    impl->driveMenu->clear();
    bool drv_enabled = false;
    int nt_idx = impl->ntmd.ntregf_index();

    const std::vector< std::map<std::string,std::string> >& drvs = impl->ntmd.windows_drives();

    if(nt_idx >=0 ){
        const auto & nt_list = impl->ntmd.get_windows();

        QAction *a = impl->driveMenu->addAction(QString::fromStdString(nt_list.at(nt_idx).nt_version()));
        a->setEnabled(false);

        for(size_t i = 0;i<drvs.size();i++){
            const std::map<std::string,std::string> &m = drvs.at(i);
            QString label = QString::fromStdString(m.at("fs_label"));
            const QString &mp = QString::fromStdString(m.at("mount_point"));
            const QString &drive_letter = QString::fromStdString(m.at("windows_drive"));
            
            if(label.length() == 0){
                label = tr("Local Disk");
            }
            const QString display_label = label + "("+drive_letter+")";
            QAction *a = impl->driveMenu->addAction(display_label);
            a->setData(QVariant::fromValue(mp));
            a->setToolTip(mp);
            drv_enabled = true;
        }
        impl->mainWin->selectWindows(nt_idx);
    }
    impl->mainWin->setWindowsDriveList(&drvs);
    impl->driveMenu->setEnabled(drv_enabled);
}


bool QApp::event(QEvent *e)
{
    if(e->type() == QEvent::ApplicationActivated){
        if(impl->_is_exec){
            showMainWindow();
        }
    }
    return QApplication::event(e);
}
