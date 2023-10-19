
#include <QLocale>
#include <QTranslator>
#include "qapp.h"
#include <QDebug>
#include "config.h"

int main(int argc, char *argv[])
{
    // 必须所有其它Qt代码之前调用，否则会无效
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication::setOrganizationName(APPLICATION_ORG);
    QApplication::setApplicationName(APPLICATION_NAME);
    QApplication::setApplicationVersion(VERSION);

    QApp a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "WindowsDrive_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            QCoreApplication::installTranslator(&translator);
            break;
        }
    }

#ifndef __APPLE__
    int ret = a.checkInstance();
    if(ret != 0){
        return ret;
    }
#endif

    return a.exec();
}
