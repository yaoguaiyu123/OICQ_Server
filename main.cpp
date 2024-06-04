#include "widget.h"
#include "tcpserver.h"
#include <QApplication>
#include "database/dbmanager.h"
#include <QDebug>
#include <QDir>
#include "fileserver.h"
#include <QThread>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    DBManager::singleTon().createTableFriendRequests();
    TcpServer &server = TcpServer::singleTon();
    FileServer fileserver;
    return a.exec();
}
