#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
struct MESG;
#include <QJsonValue>;
#include <QImage>

//负责维护一个客户端的通信
class QTcpSocket;
class QJsonObject;
class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket * tcpSocket,QObject* parent = nullptr);
    qint64 userId();
    void sendToClient(MESG *);
    void packingMessage(QJsonValue jsonvalue, int msgType, QList<QImage> imageList = QList<QImage>());
private slots:
    void on_ready_read();
signals:
    void disconnected(int);
    void transpond(QJsonValue,qint64,QList<QImage>);
    void addFriend(qint64, QString, qint64, QList<QImage>&);
    void addFriendRes(QJsonValue jsonvalue,qint64 friendId, QList<QImage>&);
private:
    QTcpSocket* m_tcpSocket;
    qint64 m_userId = -1;
    uchar* m_sendbuf;
    QByteArray m_recvbuf;
private:
    void parsePrivateMessage(QJsonValue, QList<QImage> images);
    void parseLogin(QJsonValue);
    void parseAddFriend(QJsonValue jsonvalue);
    // QJsonObject byteArrayToJson(const QByteArray&);
    void parseFriendList(qint64 userId);
    void parseUpdateHead(qint64 userId,  QList<QImage> images);
    void parseAddFriendRes(QJsonValue jsonvalue);
    void parseFriendRequestList();
};

#endif // CLIENTHANDLER_H
