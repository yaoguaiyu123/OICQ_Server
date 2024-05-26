#include "clienthandler.h"
#include <QByteArray>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtEndian>
#include "global.h"
#include "database/dbmanager.h"
#include <QBuffer>
#include <QImage>

ClientHandler::ClientHandler(QTcpSocket* tcpSocket, QObject* parent)
    : QObject(parent)
    , m_tcpSocket(tcpSocket)
{
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ClientHandler::on_ready_read);
    connect(m_tcpSocket, &QTcpSocket::disconnected,[this]() {
        emit(disconnected(m_userId));
    });
    m_sendbuf = new uchar[16 * 1024 * 1024];
}

qint64 ClientHandler::userId()
{
    return m_userId;
}

// 将信息打包成为一个MESG对象
void ClientHandler::packingMessage(QJsonValue value, int msgType,  QList<QImage> imageList)
{
    qDebug() << "打包对象: " << value;
    QJsonDocument doc;
    if (value.isObject()) {
        doc = QJsonDocument(value.toObject());
    } else if (value.isArray()){
        doc = QJsonDocument(value.toArray());
    } else {
        return;
    }
    QByteArray bytes = doc.toJson();

    MESG* message = new MESG();
    message->msg_type = static_cast<MSG_TYPE>(msgType);
    message->len = bytes.size();
    message->data = new uchar[message->len];
    std::memcpy(message->data, bytes.data(), message->len);
    //打包图片
    if (!imageList.isEmpty()) {
        for (const QImage& image : imageList) {
            QByteArray buffer;
            QBuffer qBuffer(&buffer);
            qBuffer.open(QIODevice::WriteOnly);

           // 将图片以PNG格式转换并存储到buffer中
            if (!image.save(&qBuffer, "PNG")) {
                qDebug() << "图片转换失败";
                continue;
            }
            message->imagesData.append(buffer);
        }
    }

    sendToClient(message);
}

//发送数据的函数
void ClientHandler::sendToClient(MESG* send)
{
    // qDebug() << "开始回送数据到客户端";
    if (m_tcpSocket->state() == QAbstractSocket::UnconnectedState) {
        if (send->data)
            free(send->data);
        if (send)
            free(send);
        return;
    }

    quint64 bytestowrite = 0;
    m_sendbuf[bytestowrite++] = '$'; // 开头
    qToBigEndian<quint16>(send->msg_type, m_sendbuf + bytestowrite); // quint16 决定了存入两个字节的数据
    bytestowrite += 2;
    qToBigEndian<quint64>(m_userId, m_sendbuf + bytestowrite);
    bytestowrite += 8;
    qToBigEndian<quint32>(send->len, m_sendbuf + bytestowrite);
    bytestowrite += 4;
    memcpy(m_sendbuf + bytestowrite, send->data, send->len);
    bytestowrite += send->len;

           // 图片数量
    qToBigEndian<quint32>(send->imagesData.size(), m_sendbuf + bytestowrite);
    bytestowrite += 4;

           // 图片数据
    for (const QByteArray &imgData : send->imagesData) {
        qToBigEndian<quint32>(imgData.size(), m_sendbuf + bytestowrite);
        bytestowrite += 4;
        memcpy(m_sendbuf + bytestowrite, imgData.constData(), imgData.size());
        bytestowrite += imgData.size();
    }

    m_sendbuf[bytestowrite++] = '#'; // 结尾

    qint64 hastowrite = bytestowrite;
    qint64 ret = 0, haswrite = 0;
    do {
        ret = m_tcpSocket->write((char*)m_sendbuf + haswrite, hastowrite - haswrite);
        m_tcpSocket->waitForBytesWritten();
        if (ret == -1 &&  m_tcpSocket->error() == QAbstractSocket::TemporaryError) {
            ret = 0;
        } else if (ret == -1) {
            break;
        }
        haswrite += ret;
    } while (haswrite < hastowrite); // 确保所有数据都被写入

    qDebug() << bytestowrite << "   这是我发送的数据的总数!!!!!!!!!!";

    if (send->data) {
        free(send->data);
    }
    if (send) {
        free(send);
    }
}

// 接收数据的函数
/**
 1个字节的开头 $
 2个字节的 msg_type(大端格式)
 8个字节的 userid(大端格式)
 4个字节的 len(大端格式)
 len个字节的 data
 1个字节的结尾 #
*/
void ClientHandler::on_ready_read()
{
    qDebug() << "开始读取客户端数据";

    while (m_tcpSocket->bytesAvailable() > 0) {
        int aliasSize = m_tcpSocket->bytesAvailable();
        QByteArray datagram;
        datagram.resize(aliasSize);
        m_tcpSocket->read(datagram.data(), aliasSize); // 读取数据

        qDebug() << aliasSize << "    这是我接收的数据的总数";

        if (datagram.at(0) == '$') {
            // 读到开头
            m_recvbuf.append(datagram);   //append会自动扩容
            qDebug() << "读到开头" << datagram.at(0) << " " << m_recvbuf.at(0);
        } else {
            m_recvbuf.append(datagram);
        }

        if (datagram.at(aliasSize - 1) == '#') {
            qDebug() << "读到结尾";
            break;
        } else {
            return;
        }
    }

    //解析数据包
    int pos = 0;
    char beginC = m_recvbuf.at(pos++);
    if (beginC != '$') {
        qDebug() << "开头错误 " << beginC;
        return;
    }
    quint16 msgType = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(m_recvbuf.constData() + pos));
    pos += sizeof(quint16); // 消息类型
    quint64 userId = qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(m_recvbuf.constData() + pos));
    pos += sizeof(quint64); // id
    quint32 msgLen = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_recvbuf.constData() + pos));
    pos += sizeof(quint32); // json长度
    QByteArray msgData = m_recvbuf.mid(pos, msgLen);
    pos += msgLen; // json
    quint32 imgCount = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_recvbuf.constData() + pos));
    pos += sizeof(quint32); // 读取图片数量
    // 读取图片存储到images
    QList<QImage> images;
    for (quint32 i = 0; i < imgCount; ++i) {
        quint32 imgSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_recvbuf.constData() + pos));
        pos += sizeof(quint32);
        QByteArray imgData = m_recvbuf.mid(pos, imgSize);
        pos += imgSize;
        QImage image;
        if (image.loadFromData(imgData)) {
            images.append(image);
        }
    }

    if (m_recvbuf.at(pos) != '#') {
        qDebug() << "数据包结尾错误";
        return;
    }


    qDebug() << "msgType:" << msgType;
    switch (msgType) {
    case PrivateMessage:
        parsePrivateMessage(byteArrayToJson(msgData), images);
        break;
    case GroupMessage:
        break;
    case AddFriend:
        parseAddFriend(byteArrayToJson(msgData));
        break;
    case Login:
        parseLogin(byteArrayToJson(msgData));
        break;
    case FriendList:
        parseFriendList(userId);
        break;
    case UpdateHead:
        parseUpdateHead(userId, images);
        break;
    case AddFriendRes:
        parseAddFriendRes(byteArrayToJson(msgData));
        break;
    case FriendRequestList:
        parseFriendRequestList();
        break;
    default:
        break;
    }

    m_recvbuf.clear();
}

//私发消息
void ClientHandler::parsePrivateMessage(QJsonValue jsonvalue, QList<QImage> images)
{
    // qDebug() << "socket接收到私发消息";
    emit(transpond(jsonvalue, m_userId,images));
}

//登录
void ClientHandler::parseLogin(QJsonValue jsonvalue)
{
    QJsonObject object = jsonvalue.toObject();
    qint64 userid = object.value("userId").toInteger();
    qDebug() << "登录的userid:" << userid;
    QString password = object.value("password").toString();
    QJsonObject sendObj;
    QVariantMap vmap = DBManager::singleTon().queryDataUser(userid);
    QList<QImage> imageList;
    if (!vmap.isEmpty()) {
        if (vmap.value("password").toString() == password) {
            m_userId = userid;
            sendObj["res"] = Success;
            sendObj["userId"] = userid;
            // 返回头像
            // QImage image(":/Image/default_head.png");
            QImage image(vmap.value("headpath").toString());
            if (image.isNull()) {
                qDebug() << "登录返回头像失败 " << vmap.value("headpath").toString();
                return;
            }
            imageList.append(image);
            packingMessage(sendObj, Login, imageList);
            return;
        }
    }
    sendObj["res"] = Fail;
    packingMessage(sendObj, Login);
}

// 添加好友
void ClientHandler::parseAddFriend(QJsonValue jsonvalue)
{
    if (!jsonvalue.isObject()) {
        return;
    }
    QJsonObject object = jsonvalue.toObject();
    qint64 friendId = object.value("friendId").toInteger();
    QVariantMap vmap = DBManager::singleTon().queryDataUser(friendId);
    QList<QImage> imageList;
    if (vmap.isEmpty()) {
        // 该用户不存在
        QJsonObject sendObj;
        sendObj.insert("res", Fail);
        packingMessage(sendObj, AddFriend);
    } else if (friendId == m_userId || DBManager::singleTon().queryDataFriend(m_userId,friendId)) {
        // 已经是好友了或是自己
        QJsonObject sendObj;
        sendObj.insert("res", Repeat);
        packingMessage(sendObj, AddFriend);
    } else {
        // 已经发送过好友请求且对方还没应答
        QVariantMap judgemap = DBManager::singleTon().queryFriendRequest(m_userId, friendId);
        if (!judgemap.isEmpty()) {
            QJsonObject sendObj;
            sendObj.insert("res", Repeat2);
            packingMessage(sendObj, AddFriend);
            return;
        }
        QJsonObject sendObj;
        sendObj.insert("res", Success);
        packingMessage(sendObj, AddFriend);
        QVariantMap map1 = DBManager::singleTon().queryDataUser(m_userId);  //查询本人信息，发送给转发
        QString username = map1.value("username").toString();
        QString headpath = map1.value("headpath").toString();
        QImage image(headpath);
        imageList.append(image);
        DBManager::singleTon().insertFriendRequest(m_userId, friendId);  //在请求表中插入数据
        emit(addFriend(friendId,username, m_userId,imageList));
    }
}

// 获取好友列表
void ClientHandler::parseFriendList(qint64 userId)
{
    QJsonArray sendObj;
    QList<QVariantMap> list = DBManager::singleTon().queryDataFriends(userId);
    qDebug() << userId << "的好友数量" << list.length();
    QList<QImage> imagelist;
    for (const auto& e : list) {
        qint64 fid = e.value("friendid").toLongLong();
        QVariantMap inmap = DBManager::singleTon().queryDataUser(fid);
        QString username = inmap.value("username").toString();
        // qDebug() << username;
        QString headpath = inmap.value("headpath").toString();
        QJsonObject obj;
        obj.insert("userId", fid);
        obj.insert("username", username);
        QImage image;
        image.load(headpath);
        if (image.isNull()) {
            continue;
        }
        qDebug() << "返回好友列表头像 " << headpath;
        imagelist.append(image);
        sendObj.append(obj);
    }
    packingMessage(sendObj, FriendList, imagelist);
}

// 更新头像
void ClientHandler::parseUpdateHead(qint64 userId, QList<QImage> images)
{
    qDebug() << "更新头像";
    QVariantMap inmap = DBManager::singleTon().queryDataUser(userId);
    if (!inmap.isEmpty()) {
        QString headpath = inmap.value("headpath").toString();
        QStringList strList = headpath.split("/");
        QString headname = strList.back();
        QImage image = images.at(0);
        // 保存头像
        if (!image.save("/root/.config/OICQ/server/head/" + headname)) {
            qDebug() << "保存头像失败";
        }
        // 更新数据库
        DBManager::singleTon().updateUserHeadpath(userId, "/root/.config/OICQ/server/head/" + headname);
        QJsonObject jsonObj;
        packingMessage(jsonObj, UpdateHead, images);
    }
}

// 处理添加好友的结果
void ClientHandler::parseAddFriendRes(QJsonValue jsonvalue)
{
    if (!jsonvalue.isObject()) {
        return;
    }
    QJsonObject obj = jsonvalue.toObject();
    int res = obj.value("res").toInt();
    qint64 friendId = obj.value("friendId").toInteger();
    if (res == Fail) {
        // 拒绝添加好友

    } else if (res == Success) {
        //检查是否已经是好友
        if(!DBManager::singleTon().queryDataFriend(m_userId,friendId)){
            // 添加好友,给请求加我的人发去我的信息
            QVariantMap inmap = DBManager::singleTon().queryDataUser(m_userId);
            QString headpath = inmap.value("headpath").toString();
            QString username = inmap.value("username").toString();
            QJsonObject sendObj;
            sendObj.insert("username", username);
            sendObj.insert("userId", m_userId);
            QList<QImage> images;
            QImage image(headpath);
            images.append(image);
            emit(addFriendRes(sendObj,friendId,images));
            // 将两人插入数据库
            DBManager::singleTon().insertDataFriends(friendId, m_userId);
            DBManager::singleTon().insertDataFriends(m_userId, friendId);
        }
    }
    DBManager::singleTon().deleteFriendRequest(friendId, m_userId);   //数据库中删除该条好友有请求
}

//处理获取好友添加请求列表(分多次发送)
void ClientHandler::parseFriendRequestList()
{
    qDebug() << "加载离线好友请求";
    QList<QImage> imageList;
    QList<QVariantMap> mapLists = DBManager::singleTon().queryFriendRequests(m_userId);
    for (auto& vmap : mapLists) {
        QJsonObject sendObj;
        qint64 userid = vmap.value("accountId").toLongLong();
        QVariantMap informap = DBManager::singleTon().queryDataUser(userid);
        sendObj.insert("username", informap.value("username").toString());
        sendObj.insert("userid", informap.value("accountid").toLongLong());
        QString headpath = informap.value("headpath").toString();
        QImage image(headpath);
        if (image.isNull()) {
            qDebug() << "error:image.isNull()  path = " << headpath;
        }
        imageList.append(image);
        packingMessage(sendObj, FriendRequestList , imageList);  //回送到客户端
    }
}









