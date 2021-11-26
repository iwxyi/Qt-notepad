#include <QApplication>
#include <QDebug>
#include "mainwindow.h"

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

    if (argc == 2)
    {
        QString path = QString::fromLocal8Bit(argv[1]);
        w.openFile(path);
    }

    return a.exec();
}
