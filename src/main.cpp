#include <QApplication>

#include "mainwindow.h"
#include "tracklistmodel.h"
#include "user.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz t} "
                       "%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}"
                       "C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} - %{message}");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setApplicationName("Gmusic Player");

    MainWindow window;
    window.show();

    return app.exec();
}
