#include "fileclienthandler.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include "global.h"
#include "tcpserver.h"

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

    //存放到file文件夹下
    QDir dir("/root/.config/OICQ/server");
    if (!dir.exists("file")) {
        dir.mkdir("file");
    }

    qint64 recvSize = 0, totalSize = 0 , from ,to;
    QString filename;
    QFile file;
    bool is_begin = false;

    // 最多等待30s
    while (socket.waitForReadyRead(30000)) {
        QByteArray byteArray = socket.readAll();
        if (byteArray.isEmpty()) {
            qDebug() << "empty()";
            continue;
        }
        if (byteArray[0] == 'U' && !is_begin) {
            QDataStream stream(&byteArray, QIODevice::ReadOnly);
            is_begin = true;
            byteArray.remove(0, 1); // 去除开头
            stream >> totalSize; // 读取文件大小
            QByteArray nameByte;
            stream >> nameByte; // 读取文件名数据
            filename = QString::fromUtf8(nameByte);
            stream >> from;
            stream >> to;
            file.setFileName(dir.path() + "/file/" + getRandomString(7) + "_" + filename);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "open file fail : " << file.fileName();
                return;
            } else {
                qDebug() << "开始接收" << file.fileName() << "文件";
            }
            // 去除已读取部分
            byteArray.remove(0, sizeof(qint64) + sizeof(qint32) + filename.length());
            qDebug() << "将要去除的数据包的长度(开头不算):" << sizeof(qint64) + sizeof(qint32) + filename.length();
            file.write(byteArray);
            recvSize += byteArray.size();
            qDebug() << recvSize << "   " << totalSize;
        } else if (is_begin) {
            // 继续写入文件
            file.write(byteArray);
            recvSize += byteArray.size();
            qDebug() << recvSize << "   " << totalSize;
        } else {
            // 开头错误
            qDebug() << "开头错误";
            return;
        }
    }
    if (recvSize < totalSize) {
        file.remove(); // 删除不完整的文件
        // qDebug() << "文件接收不完整，已删除";
    } else {
        qDebug() << "文件接收成功" << file.fileName();
        // 通知消息服务器进行转发

        double kilobytes = totalSize / 1024.0;
        double megabytes = totalSize / 1048576.0;

        QString result;
        if (totalSize >= 1048576) {
            result = QString::number(megabytes, 'f', 2) + " MB";
        } else if (totalSize >= 1024) {
            result = QString::number(kilobytes, 'f', 2) + " KB";
        } else {
            result = QString::number(totalSize) + " B";
        }

        TcpServer::singleTon().transferFile(from, to, filename, result);
    }

    file.close();
}



