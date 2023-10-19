#ifndef HANDLEREPLY_H
#define HANDLEREPLY_H

#include <QObject>
#include <QJsonObject>
#include <QNetworkReply>

#include "networkmanager.h"

class HandleReply : public QObject
{
    Q_OBJECT
public:
    HandleReply(QNetworkReply*reply, QObject *receiver, ReceiverFunc receiver_func);

private slots:
    void onFinished();

private:
    QNetworkReply*reply;
    QObject* receiver;
    ReceiverFunc func;
};

#endif // HANDLEREPLY_H
