#include "widget.h"
#include "tcpserver.h"
#include <QApplication>
#include "database/dbmanager.h"
#include <QDebug>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    // qDebug() << QDir::currentPath();
    DBManager::singleTon().createTableFriendRequests();
    TcpServer server;
    // Widget w;
    // w.show();
    return a.exec();
}
