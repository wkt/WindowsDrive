#ifndef PASSWDDIALOG_H
#define PASSWDDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class PasswdDialog;
}

class PasswdDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PasswdDialog(QWidget *parent = nullptr);
    ~PasswdDialog();

    QString password();
    void show();

private:
    Ui::PasswdDialog *ui;
};

#endif // PASSWDDIALOG_H
