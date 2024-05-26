#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QList>
#include <QObject>
#include <QtSql>

class DBManager : QObject {
public:
    explicit DBManager(QObject* parent = nullptr);
    ~DBManager();
    //users表
    bool insertDataUsers(qint64 accountId, const QString& username, const QString& password, const QString& headpath);
    QList<QVariantMap> queryDataUsers();
    QVariantMap queryDataUser(qint64 id);
    bool createTableUsers();
    QString queryDataUsersName(qint64 id);
    //friends表
    bool insertDataFriends(qint64 accountId, qint64 friendId);
    bool updateUserHeadpath(int accountId, const QString newHeadpath);
    QList<QVariantMap> queryDataFriends(qint64 id);
    bool queryDataFriend(qint64 accountId, qint64 friendId);
    bool createTableFriends();
    // friendRequest表
    bool createTableFriendRequests();
    QVariantMap queryFriendRequest(qint64 accountId, qint64 friendId);
    QList<QVariantMap> queryFriendRequests(qint64 friendId);
    bool insertFriendRequest(qint64 accountId, qint64 friendId);
    bool deleteFriendRequest(qint64 accountId, qint64 friendId);

    bool openDatabase(const QString& dbName);
    void closeDatabase();
    bool isTableExists(const QString& tableName);
    static DBManager& singleTon();

    QSqlDatabase m_database;

private:

};

#endif // DBMANAGER_H
