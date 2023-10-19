#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ntregf.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QJsonObject;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setWindowsList(const std::vector<NTRegf>*nt_list);
    void selectWindows(int index);
    void setWindowsDriveList(const std::vector< std::map<std::string,std::string> >* drive_list);
    void center();

    void showLoading(const QString &text = "");
    void hideLoading();
    bool isLoading();

    void showUpdateInfo(const QJsonObject* json);

protected:
    bool event(QEvent *ev);

    void setupMenubar();

private slots:
    void checkUpdate();
    void windowsChanged(int index);

private:
    struct MainWindowImpl;
    Ui::MainWindow  *ui;
    MainWindowImpl  *impl;
};
#endif // MAINWINDOW_H
