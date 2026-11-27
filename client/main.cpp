#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载全局样式表
    QFile qss(":/style.qss");
    if(qss.open(QFile::ReadOnly|QFile::Text)){
        a.setStyleSheet(qss.readAll());
        qss.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
