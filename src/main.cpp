#include <QApplication>

#include "mainwindow.h"
#include "tracklistmodel.h"
#include "user.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setApplicationName("Gmusic Player");

    MainWindow window;
    window.show();

    return app.exec();
}
