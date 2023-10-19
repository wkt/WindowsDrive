#include "passwddialog.h"
#include "ui_passwddialog.h"
#include "utils.h"


PasswdDialog::PasswdDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswdDialog)
{
    ui->setupUi(this);
    ui->username->setText(QString::fromStdString(get_user_displayname()));
}

PasswdDialog::~PasswdDialog()
{
    delete ui;
}


QString PasswdDialog::password()
{
    return ui->password->text();
}

void PasswdDialog::show()
{
    QDialog::show();
    ui->password->selectAll();
    ui->password->setFocus();
}
