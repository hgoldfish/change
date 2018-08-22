#include "changeitwidget.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLocale sysLocale = QLocale::system();
    QTranslator translator;
    if (sysLocale.language() == QLocale::Chinese) {
        if (translator.load("change.qm")) {
            app.installTranslator(&translator);
        }
    }
    ChangeItWidget w;
    w.show();
    return app.exec();
}
