#include "drivebutton.h"
#include "ui_drivebutton.h"
#include "qapp.h"

#include <QGridLayout>
#include <QBoxLayout>

DriveButton::DriveButton(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::DriveButton),
    mp("")
{
    ui->setupUi(this);
    QGridLayout *grid = new QGridLayout(this);
    QBoxLayout  *vertical = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    grid->addWidget(ui->drive_icon,0,0);
    grid->addWidget(ui->win_icon,0,0,Qt::AlignHCenter);
    ui->win_icon->setVisible(false);
    vertical->addWidget(ui->drive_label);
    vertical->addWidget(ui->drive_mountpoint);
    grid->addLayout(vertical,0,1);
    setLayout(grid);
    setAutoFillBackground(true);
    setFlat(false);
}

DriveButton::~DriveButton()
{
    delete ui;
}

void DriveButton::updateUiFrom(const std::map<std::string,std::string> &m)
{
    QString label = QString::fromStdString(m.at("fs_label"));
    const QString &_mp = QString::fromStdString(m.at("mount_point"));
    const QString &drive_letter = QString::fromStdString(m.at("windows_drive"));
    
    if(label.length() == 0){
        label = tr("Local Disk");
    }
    const QString display_label = label + "("+drive_letter+")";
    ui->drive_label->setText(display_label);
    ui->drive_mountpoint->setText(_mp);
    ui->drive_mountpoint->setToolTip(_mp);
    this->mp = _mp;
    setToolTip(tr("Click to open ")+display_label);
}

const QString& DriveButton::driveMountpoint() const
{
    return mp;
}

void DriveButton::handleClicked()
{
    qApp->handleOpenDirectory(driveMountpoint());
}

void DriveButton::showWindowsIcon(const bool& show)
{
    ui->win_icon->setVisible(show);
}

