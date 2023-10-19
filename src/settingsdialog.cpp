#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "qapp.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Drawer| Qt::Sheet);
    setWindowModality(Qt::WindowModal);
    ui->mount_checkbox->setChecked(qApp->canMountWindowsPartition());
    ui->autostart_checkbox->setChecked(qApp->canAutostart());
    ui->hideOnClose->setChecked(qApp->isHideOnClose());
    ui->checkUpdate->setChecked(qApp->isCheckUpdateOnStart());

#ifdef __APPLE__
    ui->autostart_checkbox->hide();
#endif

    QObject::connect(ui->mount_checkbox,SIGNAL(toggled(bool)),qApp,SLOT(setMountWindowsPartition(bool)));
    QObject::connect(ui->autostart_checkbox,SIGNAL(toggled(bool)),qApp,SLOT(setAutostart(bool)));
    QObject::connect(ui->hideOnClose,SIGNAL(toggled(bool)),qApp,SLOT(setHideOnClose(bool)));
    QObject::connect(ui->checkUpdate,SIGNAL(toggled(bool)),qApp,SLOT(setCheckUpdateOnStart(bool)));

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
