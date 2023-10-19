#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>

extern const char *KEY_VER;
extern const char *KEY_DATE;
extern const char *KEY_LOG;
extern const char *KEY_URL;

class QNetworkAccessManager;

typedef void (*ReceiverFunc)(QObject *receiver,const void* result);

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    virtual ~NetworkManager();

    void checkUpdate(QObject *receiver,  ReceiverFunc func);

private:
    QNetworkAccessManager *nm;
};

#endif // NETWORKMANAGER_H
