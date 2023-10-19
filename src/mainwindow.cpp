#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "drivebutton.h"
#include "qapp.h"
#include <QDebug>
#include <QEvent>
#include <QMessageBox>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonObject>
#include "config.h"
#include "networkmanager.h"
#include "loading_dialog.h"
#include "utils.h"

static const uint MAX_DIRVE_GRID_COLUMNS = 2;

struct MainWindow::MainWindowImpl{
    const std::vector<NTRegf> *nt_list;
    QGridLayout *grid_layout;
    LoadingDialog *dlgLoading;
    bool forceShowUpdateInfo;
    bool comboUpdating;

    MainWindowImpl():nt_list(NULL),
        grid_layout(new QGridLayout),
        dlgLoading(NULL),
        forceShowUpdateInfo(false),
        comboUpdating(false)
    {
    }
    
    ~MainWindowImpl()
    {
        if(dlgLoading != NULL){
            delete dlgLoading;
            dlgLoading = NULL;
        }

        delete grid_layout;
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , impl(new MainWindowImpl)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/app.svg"));
    ui->drives_box->setLayout(impl->grid_layout);
    ui->testReloadButton->setVisible(false);
    connect(ui->testReloadButton,SIGNAL(clicked()),qApp,SLOT(loadWindowsDrive()));
    connect(ui->wins_combo,SIGNAL(currentIndexChanged(int)),this,SLOT(windowsChanged(int)));
    impl->grid_layout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    setupMenubar();

}

MainWindow::~MainWindow()
{
    delete ui;
    delete impl;
}

void MainWindow::setWindowsList(const std::vector<NTRegf>* nt_list)
{
    //qDebug()<<__FUNCTION__<<"drive_list:"<<nt_list;
    impl->nt_list = nt_list;
    size_t n_list = (nt_list!=NULL?nt_list->size():0);
    impl->comboUpdating = true;
    ui->wins_combo->clear();
    if(n_list == 0){
        ui->wins_combo->addItem(tr("No Windows OS found"));
        ui->wins_combo->setCurrentIndex(0);
        ui->wins_combo->setEnabled(false);
        impl->comboUpdating = false;
        return;
    }

    ui->wins_combo->setEnabled(true);
    for(size_t i=0;i<n_list;i++){
        const NTRegf &nt = nt_list->at(i);
        const QString &nt_v = QString::fromStdString(nt.nt_version());
        const QString &dp = QString::fromStdString(nt.dev_path());
        ui->wins_combo->addItem(nt_v,QVariant::fromValue(dp));
    }
    ui->wins_combo->setCurrentIndex(-1);
    impl->comboUpdating = false;
}

void MainWindow::selectWindows(int index)
{
    //qDebug()<<__FUNCTION__<<"index: "<<index<<", currentIndex: "<<ui->wins_combo->currentIndex();
    if(index<0)return;
    if(index != ui->wins_combo->currentIndex()){
        ui->wins_combo->setCurrentIndex(index);
    }
}

void MainWindow::setWindowsDriveList(const std::vector< std::map<std::string,std::string> >* drive_list)
{
    //qDebug()<<__FUNCTION__<<"drive_list:"<<drive_list;
    size_t n_drive = drive_list?drive_list->size():0;
    QGridLayout *gridLayout = impl->grid_layout;
    while(gridLayout->count()>0){
        QLayoutItem *item = gridLayout->itemAt(0);
        gridLayout->removeItem(item);
        item->widget()->deleteLater();
        delete item;
    }
    if(n_drive == 0){
        QLabel *not_found = new QLabel(tr("No windows drive found"));
        not_found->setEnabled(false);
        gridLayout->addWidget(not_found,0,0);
        return;
    }
    ui->drives_box->update();
    const NTRegf& nt = impl->nt_list->at(ui->wins_combo->currentIndex());
    for(size_t i=0;i<n_drive;i++){
        DriveButton *db = new DriveButton(ui->drives_box);
        int c = i% MAX_DIRVE_GRID_COLUMNS;
        int r = int(i/MAX_DIRVE_GRID_COLUMNS);
        gridLayout->addWidget(db,r,c);
        const std::map<std::string,std::string> &m = drive_list->at(i);
        db->updateUiFrom(m);
        if(nt.dev_path() == map_get<std::string,std::string>(m,"partition")){
            db->showWindowsIcon(true)
;        }
        db->connect(db,SIGNAL(clicked()),SLOT(handleClicked()));
    }
}

bool MainWindow::event(QEvent *ev)
{
    //qDebug()<<__PRETTY_FUNCTION__<<",ev:"<<ev->type();
    if(ev->type() == QEvent::Close){
        if(qApp->isHideOnClose()){
            hide();
            ev->ignore();
            return true;
        }
    }
    return QMainWindow::event(ev);
}

void MainWindow::center()
{
    setGeometry(
        QStyle::alignedRect( 
            Qt::LeftToRight,
            Qt::AlignCenter,
            this->size(),
            qApp->primaryScreen()->availableGeometry()
        )
    );
}

void MainWindow::showLoading(const QString &text)
{
    if(impl->dlgLoading == NULL){
        impl->dlgLoading = new LoadingDialog(this);
        impl->dlgLoading->setWindowFlags(Qt::Dialog |Qt::Drawer| Qt::Sheet|Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
        impl->dlgLoading->setModal(true);
        impl->dlgLoading->setWindowModality(Qt::WindowModal);
#ifndef __APPLE__
        impl->dlgLoading->setAttribute(Qt::WA_TranslucentBackground);
#endif
        impl->dlgLoading->setStyleSheet("QDialog{min-width:240px;}");
    }
    impl->dlgLoading->show(text);
}

void MainWindow::hideLoading()
{
    if(impl->dlgLoading){
        impl->dlgLoading->setVisible(false);
        impl->dlgLoading->hide();
    }
}

bool MainWindow::isLoading()
{
    if(impl->dlgLoading){
        return impl->dlgLoading->isVisible() || (! impl->dlgLoading->isHidden());
    }
    return false;
}

void MainWindow::setupMenubar()
{
    QMenuBar *menubar = menuBar();

    QMenu  *fileMenu = menubar->addMenu(tr("&File"));

    QAction *refreshAct;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    refreshAct = fileMenu->addAction(tr("&Reload"),qApp,&QApp::loadWindowsDrive,QKeySequence::StandardKey::Refresh);
#else
    refreshAct = fileMenu->addAction(tr("&Reload"));
    connect(refreshAct,&QAction::triggered,qApp,&QApp::loadWindowsDrive);
#endif
    QList<QKeySequence> keys = {QKeySequence::StandardKey::Refresh,QKeySequence("F5")};
    refreshAct->setShortcuts(keys);
    refreshAct->setMenuRole(QAction::NoRole);

    QAction *quitAct;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    quitAct = fileMenu->addAction(tr("&Quit"),qApp,&QApp::handleQuit,QKeySequence::Quit);
#else
    quitAct = fileMenu->addAction(tr("&Quit"));
    connect(quitAct,&QAction::triggered,qApp,&QApp::handleQuit);
#endif
    quitAct->setMenuRole(QAction::QuitRole);

    QMenu  *editMenu = menubar->addMenu(tr("&Edit"));

    QAction *actP;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    actP = editMenu->addAction(tr("&Preferences..."),qApp,&QApp::showSettingsDialog,QKeySequence::Preferences);
#else
    actP = editMenu->addAction(tr("&Preferences..."));
    connect(actP,&QAction::triggered,qApp,&QApp::showSettingsDialog);
#endif
    actP->setMenuRole(QAction::PreferencesRole);

    QMenu *helpMenu = menubar->addMenu(tr("&Help"));

    QAction *aboutAct;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    aboutAct = helpMenu->addAction(tr("About"), qApp, &QApp::showAbout);
#else
    aboutAct = helpMenu->addAction(tr("About"));
    connect(aboutAct,&QAction::triggered,qApp,&QApp::showAbout);
#endif
    aboutAct->setMenuRole(QAction::AboutRole);

#if defined(SHOW_ABOUT_QT) && SHOW_ABOUT_QT
    QAction* aboutQt = helpMenu->addAction(tr("About Qt"));
    connect(aboutQt,&QAction::triggered,qApp,&QApp::aboutQt);
#endif

    QAction *checkAction;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    checkAction = helpMenu->addAction(tr("Check Update"),this,&MainWindow::checkUpdate);
#else
    checkAction = helpMenu->addAction(tr("Check Update"));
    connect(checkAction,&QAction::triggered,this,&MainWindow::checkUpdate);
#endif
    checkAction->setStatusTip(tr("Check For update"));

}

void MainWindow::checkUpdate()
{
    impl->forceShowUpdateInfo = true;
    qApp->checkUpdate();
}

void MainWindow::showUpdateInfo(const QJsonObject* _json)
{
    const QJsonObject &json = *_json;
    bool canShow = false;
    QString ti = tr("Up To Date");
    QString msg = "";
    QString ver = json.value(KEY_VER).toString();
    if(ver.size()>0){
        ti= tr("Found new version: ")+ver;
        msg = tr("Change Log(s):") +"\n    "+json.value(KEY_LOG).toString().replace("\n","\n    ");
        canShow = true;
    }
    if(impl->forceShowUpdateInfo){
        canShow = true;
    }

    impl->forceShowUpdateInfo = false;
    if(!canShow){
        return;
    }

    QMessageBox msgBox(this);

    QString title = tr("Windows Drive");

    msgBox.setWindowTitle(title);
    msgBox.setWindowFlags((msgBox.windowFlags()& ~Qt::WindowType_Mask)|Qt::Sheet);
    msgBox.setText(ti);
    QMessageBox::StandardButtons sb = QMessageBox::Cancel;
    msgBox.setIcon(QMessageBox::NoIcon);
    if(msg.size()>0){
        msgBox.setDetailedText(msg);
    }else{
        sb = QMessageBox::Ok;
        msgBox.setInformativeText(tr("No update(s) found"));
        //change size of the dialog
        msgBox.setStyleSheet("QDialogButtonBox{min-width:240px;}");
    }
    QString url = json.value(KEY_URL).toString();
    if(url.trimmed().size()>=8){
        sb = sb | QMessageBox::QMessageBox::Yes;
    }
    msgBox.setStandardButtons(sb);
    if (sb & QMessageBox::Yes)
        msgBox.button(QMessageBox::Yes)->setText(tr("Update"));
    msgBox.setTextFormat(Qt::RichText);

    show();
    raise();

    int ret = msgBox.exec();
    if(ret == QMessageBox::Yes){
        QDesktopServices::openUrl(QUrl(url));
    }
}

void MainWindow::windowsChanged(int index)
{
    if(impl->comboUpdating)return;
    if(index<0){
        setWindowsDriveList(NULL);
        return;
    }
    if(impl->nt_list == NULL || impl->nt_list->size() == 0)return;
    const std::string& dev = impl->nt_list->at(index).dev_path();
    qApp->setSelectedWindowDevice(QString::fromStdString(dev));
}
