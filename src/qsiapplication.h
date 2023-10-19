#ifndef QSIAPPLICATION_H
#define QSIAPPLICATION_H

#include <QApplication>
#include <QCommandLineParser>

/**
 * Single Instance Application
 * @brief The QSIApplication class
 */
class QSIApplication : public QApplication
{
    Q_OBJECT
public:
    QSIApplication(int &argc, char **argv, int flags = ApplicationFlags);
    ~QSIApplication();

    /* check if already has a instance */
    virtual int checkInstance() final;


protected:
    virtual void handleAddOptions(QCommandLineParser &parser) = 0;
    
    /**
     * handle 
     * @brief handleArguments
     * @param parser
     * @return 0 for normal
     */
    virtual int handleArguments(const QCommandLineParser &parser) = 0;

private slots:
    void handleNewArguments();
    int processArguments(const bool& allow_unknown_option=false);

private:
    class Impl;
    Impl *impl;
};

#endif // QSIAPPLICATION_H
