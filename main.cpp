#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont f(a.font());
    f.setFamily("微软雅黑");
    a.setFont(f);

    a.setApplicationName("notepad");
    a.setApplicationVersion("v0.1");
    a.setApplicationDisplayName("记事本");

    MainWindow w;
    w.show();
    return a.exec();
}
