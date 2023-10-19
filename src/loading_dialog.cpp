#include "loading_dialog.h"

#include <QLabel>
#include <QMovie>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QSize>

class LoadingDialog::Impl
{
    
public:
    Impl(LoadingDialog *dlg):
        gif(new QMovie(":icons/loading.gif")),
        gifWidget(new QLabel(dlg)),
        msgLabel(new QLabel(dlg))
    {
        gifWidget->setMovie(gif);
    }

    ~Impl(){
        delete gif;
        delete gifWidget;
        delete msgLabel;
    }

    friend class LoadingDialog;
    
protected:
    QMovie *gif;
    QLabel *gifWidget;
    QLabel *msgLabel;
    
};

LoadingDialog::LoadingDialog(QWidget *parent, Qt::WindowFlags f)
    :QDialog(parent,f)
    ,impl(new Impl(this))
{
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(impl->gifWidget);
    vbox->addWidget(impl->msgLabel);
    impl->gifWidget->setScaledContents(true);
    
    impl->gifWidget->setFixedSize(QSize(64,64));
    QSizePolicy splc = impl->gifWidget->sizePolicy();
    splc.setVerticalPolicy(QSizePolicy::Fixed);
    splc.setHorizontalPolicy(QSizePolicy::Fixed);
    
    setLayout(vbox);
    vbox->setAlignment(Qt::AlignCenter);
    impl->msgLabel->setAlignment(Qt::AlignCenter);
    impl->msgLabel->setEnabled(false);
    impl->gifWidget->setEnabled(false);
}

LoadingDialog::~LoadingDialog()
{
    delete impl;
}

void LoadingDialog::show(const QString &text)
{
    impl->gif->start();
    impl->msgLabel->setText(text);
    impl->msgLabel->setVisible(text.length()>0);
    QDialog::show();
}

void LoadingDialog::hide()
{
    impl->gif->stop();
    QDialog::hide();
}

