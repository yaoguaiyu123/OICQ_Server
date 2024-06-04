#ifndef GLOBAL_H
#define GLOBAL_H
#include <QList>
#include <QString>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QRandomGenerator>

QString getRandomString(int nLen);



QJsonValue byteArrayToJson(const QByteArray& jsonData);


struct Recode {
    QString date;
    QString type;
    QString message;
};

struct Friend {
    QString name;
    QString headPath;
    QList<Recode>* recode;
    qint64 userid; // QQ号
};

enum MSG_TYPE {
    PrivateMessage,
    GroupMessage,
    AddFriend,
    Login,
    FriendList,
    UpdateHead,
    AddFriendRequest,
    AddFriendRes,
    FriendRequestList
};

enum RETURN{
    Repeat,
    Success,
    Fail,
    Repeat2
};

struct MESG //消息结构体
{
    MSG_TYPE msg_type;
    uchar* data;
    int len;
    QList<QByteArray> imagesData;
};
Q_DECLARE_METATYPE(MESG *);

#ifndef MB
#define MB 1024*1024
#endif

#ifndef KB
#define KB 1024
#endif


#endif // GLOBAL_H
