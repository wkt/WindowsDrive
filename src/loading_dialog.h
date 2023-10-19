#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QDialog>
#include <QString>

class LoadingDialog : public QDialog
{
    Q_OBJECT
public:
    LoadingDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~LoadingDialog();
    
    virtual void show(const QString& msg);
    virtual void hide();
    
private:
    class Impl;
    Impl *impl;
};

#endif // LOADINGDIALOG_H
