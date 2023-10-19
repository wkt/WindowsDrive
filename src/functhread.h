#ifndef FUNCTHREAD_H
#define FUNCTHREAD_H

#include <QThread>
#include <QObject>


typedef void (*ThreadFunc)(void*,void *);

class FuncThread : public QThread
{
    Q_OBJECT
public:
    explicit FuncThread(ThreadFunc func,void *func_arg0,void*func_arg1=NULL , QObject *parent = nullptr);
    virtual ~FuncThread();

protected:
     void run() override;

private:
    ThreadFunc func;
    void *arg0;
    void *arg1;
};

#endif // FUNCTHREAD_H
