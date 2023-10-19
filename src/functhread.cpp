#include "functhread.h"
#include <QDebug>

FuncThread::FuncThread(ThreadFunc func,void *func_arg0,void*func_arg1,QObject *parent)
    : QThread{parent},func(func),arg0(func_arg0),arg1(func_arg1)
{
    
}

FuncThread::~FuncThread()
{
    func = NULL;
    arg0 = NULL;
    arg1 = NULL;
}

void FuncThread::run()
{
    if(func){
        func(arg0,arg1);
    }
}
