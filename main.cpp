#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序名称和组织，有助于Qt存储设置
    a.setApplicationName("AudioPlayer");
    a.setOrganizationName("BitZion");

    MainWindow w;
    w.show();
    return a.exec();
}
