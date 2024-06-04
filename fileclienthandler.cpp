#include "fileclienthandler.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include "global.h"

FileClientHandler::FileClientHandler(qintptr descriptor)
    : socketDescriptor(descriptor)
{
}

void FileClientHandler::run()
{
    QTcpSocket socket;
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        qDebug() << "socket descriptor setup failed";
        return;
    }

    QDir dir("/root/.config/OICQ/server");
    if (!dir.exists("file")) {
        dir.mkdir("file");
    }

    qint64 recvSize = 0, totalSize = 0;
    QString filename;
    QFile file;
    bool is_begin = false;
    qint32 nameLen = 0;

    while (socket.waitForReadyRead()) {
        QByteArray byteArray = socket.readAll();
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        if (byteArray.isEmpty()) {
            continue;
        }
        if (byteArray[0] == 'U' && !is_begin) {
            // 读出开头和大小
            is_begin = true;
            byteArray.remove(0, 1); // 读出开头
            stream >> totalSize; // 文件总大小
            stream >> nameLen;   // 文件名长度
           // 读取文件名
            QByteArray nameArray;
            for (int i = 0; i < nameLen; ++i) {
                char c;
                stream >> c;
                nameArray.append(c);
            }
            filename = QString::fromUtf8(nameArray);
            file.setFileName(dir.filePath(filename));
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "open file fail";
                return;
            }
            // 去除已读取部分
            byteArray = byteArray.mid(sizeof(qint64) + sizeof(qint32) + nameLen);
            file.write(byteArray);
            recvSize += byteArray.size();
        } else {
            // 继续写入文件
            file.write(byteArray);
            recvSize += byteArray.size();
        }
    }

    if (recvSize < totalSize) {
        file.remove(); // 删除不完整的文件
        qDebug() << "文件接收不完整，已删除";
    } else {
        qDebug() << "文件接收成功";
    }
    file.close();
}



