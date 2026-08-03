#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <cstdio>
#include <QtGlobal>

void outputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    fprintf(stderr, "[QT] %s\n", qPrintable(msg));
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(outputHandler);
    QApplication a(argc, argv);
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "AA-Tool_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    QApplication::setOrganizationName("misha");
    QApplication::setApplicationName("AAQT");
    MainWindow w;
    w.show();
    return QApplication::exec();
}