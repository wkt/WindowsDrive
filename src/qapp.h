#ifndef QAPP_H
#define QAPP_H

#include "ntregf.h"
#include <QAction>
#include "qsiapplication.h"



class QApp : public QSIApplication
{
    Q_OBJECT
public:
    QApp(int &argc, char **argv);
    ~QApp();

    int exec();

    bool canMountWindowsPartition();

    const QString selectedWindowDevice();
    void setSelectedWindowDevice(const QString& dev_path);
    const std::vector<NTRegf>& get_windows();
    bool canAutostart();
    bool isHideOnClose();
    bool isCheckUpdateOnStart();

public slots:
    void loadWindowsDrive();
    void handleLoadWindowsDriveFinished();
    void setMountWindowsPartition(bool enabled);

    void showAbout();
    void showMainWindow();

    void handleQuit();
    void showSettingsDialog();
    void handleAction(QAction *aciton);

    void setAutostart(const bool& enabled);
    void handleOpenDirectory(const QString &dirpath);
    void setHideOnClose(bool enabled);

    void checkUpdate();
    void setCheckUpdateOnStart(const bool& enabled);
    void updateWindowsDriveUI();

protected:
    virtual void handleAddOptions(QCommandLineParser &parser) override;
    virtual int handleArguments(const QCommandLineParser &parser) override;
    virtual bool event(QEvent *e)override;

    bool authenticationRequried();

private:
    class QAppImpl;
    QAppImpl *impl;
    
    void load_window_drive_thread();
    void authentication_verify(const QString* auth);

};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QApp*>(QCoreApplication::instance()))


#define KEY_SELECTED_WINDOW_DEV_PATH "selectedWindowDevice"
#define KEY_MOUNT_WINDOW_PARTITION "mountBeforeScanPartition"
#define KEY_AUTOSTART "is_autostart_enabled"
#define KEY_HIDE_ON_CLOSE "hideOnClose"
#define KEY_ON_START_CHECK_UPDATE "onStartCheckUpdate"


#endif // QAPP_H
