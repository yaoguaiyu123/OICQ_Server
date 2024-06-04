#include "tcpserver.h"
#include <QTcpServer>
#include <QDebug>
#include "clienthandler.h"
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtEndian>
#include "global.h"
#include <QThread>

TcpServer::TcpServer(QObject* parent)
    : QTcpServer { parent }
{
    qDebug() << tr("消息服务器开始在%1端口监听").arg(8080);
    startListen(8080);  //在8080端口进行监听
}

TcpServer::~TcpServer()
{
    closeListen();
}

bool TcpServer::startListen(int port)
{
    closeListen();
    // 开始在port端口进行监听
    return listen(QHostAddress::Any, port);
}

void TcpServer::closeListen()
{
    if (isListening()) {
        close();
    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "服务端获取一个新的连接,正在创建套接字.....";
    QTcpSocket* socket = new QTcpSocket();
    socket->setSocketDescriptor(socketDescriptor);
    QThread* thread = new QThread;
    socket->moveToThread(thread);   //在一个线程中运行socket
    thread->start();
    threadList.append(thread);
    ClientHandler* clientHandler = new ClientHandler(socket, this);
    connect(clientHandler, &ClientHandler::disconnected, this, &TcpServer::on_disconnected);
    connect(clientHandler, &ClientHandler::transpond, this, &TcpServer::on_transpond);
    connect(clientHandler, &ClientHandler::addFriend, this, &TcpServer::on_addFriend);
    connect(clientHandler, &ClientHandler::addFriendRes, this, &TcpServer::on_addFriendRes);
    socketList.append(clientHandler);
}

// 断开连接
void TcpServer::on_disconnected(int id)
{
    for (int i = 0; i < socketList.length(); ++i) {
        if (socketList.at(i)->userId() == id){
            delete socketList[i];
            socketList.remove(i);
            // 将线程对象也释放
            threadList[i]->quit();
            threadList[i]->wait();
            delete threadList[i];
            threadList.remove(i);
            qDebug() << QString("用户%1下线").arg(id);
        }
    }
}


// 进行消息转发的槽函数
// 根据from to发送消息
void TcpServer::on_transpond(QJsonValue jsonvalue,qint64 from,QList<QImage> images)
{
    if (!jsonvalue.isObject()) {
        return;
    }
    QJsonObject object = jsonvalue.toObject();
    qDebug() << "server接收到转发消息:" + object.value("message").toString();
    qint64 to = object.value("to").toInteger();
    qDebug() << "to:" << to;
    // TODO 优化为Map查找 添加好友判断
    for (auto& userSocket : socketList) {
        if (userSocket->userId() == to) {
            QJsonObject sendObj;
            sendObj.insert("from", from);
            sendObj.insert("message", object.value("message").toString());
            userSocket->packingMessage(sendObj, PrivateMessage, images);
            break;
        }
    }
}

// 转发好友添加
void TcpServer::on_addFriend(qint64 to, QString fromname, qint64 from, QList<QImage> &imagelist)
{
    qDebug() << "接收到添加好友的信息 " << from << " to " << to;
    for (auto& userSocket : socketList) {
        if (userSocket->userId() == to) {
            QJsonObject sendObj;
            sendObj.insert("fromname", fromname);
            sendObj.insert("fromid", from);
            userSocket->packingMessage(sendObj, AddFriendRequest,imagelist);
            break;
        }
    }
}

//处理好友添加的结果
void TcpServer::on_addFriendRes(QJsonValue jsonvalue,qint64 friendId, QList<QImage>& images){
    for (auto& userSocket : socketList) {
        if (userSocket->userId() == friendId) {
            userSocket->packingMessage(jsonvalue, AddFriendRes,images);
            break;
        }
    }
}





