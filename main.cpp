#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QNetworkInterface>
#include <QThread>
#include "database/dbmanager.h"
#include "fileserver.h"
#include "tcpserver.h"
#include "widget.h"
//
namespace {
void printLocalIPs()
{
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QNetworkInterface &networkInterface : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry &entry : networkInterface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && entry.ip() != localhost) {
                qDebug() << "我的IP地址: " << entry.ip().toString();
            }
        }
    }
}
} // namespace

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    printLocalIPs();
    // DBManager::singleTon().createTableFriendRequests();
    // DBManager::singleTon().createTableFiles();  //创建文件表
    DBManager::singleTon().createTableMessages();  //创建消息表
    TcpServer &server = TcpServer::singleTon();
    FileServer fileserver;
    return a.exec();
}
