#include "handlereply.h"
#include <QJsonDocument>
#include <QDebug>
#include "utils.h"

HandleReply::HandleReply(QNetworkReply*reply, QObject *receiver, ReceiverFunc receiver_func):
    QObject(NULL),reply{reply},receiver{receiver},func{receiver_func}
{
    connect(reply,&QNetworkReply::finished,this,&HandleReply::onFinished);
}

void HandleReply::onFinished()
{

    QJsonObject json;
    if(reply->error() == QNetworkReply::NoError){
        QByteArray data = reply->readAll();
        if(data.size()>0){
            QJsonDocument json_doc = QJsonDocument::fromJson(data);
            json = json_doc.object();
        }
        if(json.value("code").toInt(-1) != 0 && is_debug_mode()){
            qWarning()<<"HandleReply data: "<<data.constData();
        }
    }else{
        if(is_debug_mode())qWarning()<<"HandleReply: "<<reply->errorString();
    }
    if(json.size() == 0){
        json.insert("code",90009);
        json.insert("msg",reply->errorString());
    }

    func(receiver,&json);
    reply->close();
    reply->deleteLater();
    deleteLater();
}
