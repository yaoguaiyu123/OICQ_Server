#include "fileclienthandler.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include "global.h"
#include "tcpserver.h"
#include "database/dbmanager.h"
#include <QEventLoop>

#define maxBlock 1024

FileClientHandler::FileClientHandler(qintptr descriptor)
    : socketDescriptor(descriptor)
    , haveWritten(0)
    , toWrite(0)
{
}

// 处理 下载文件开头 或者 上传文件 的函数
void FileClientHandler::run()
{
    m_socket = new QTcpSocket;
    if (!m_socket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "设置失败 ";
        return;
    }

    //存放到file文件夹下
    QDir dir("/root/.config/OICQ/server");
    if (!dir.exists("file")) {
        dir.mkdir("file");
    }

    qint64 recvSize = 0, totalSize = 0 , from ,to , messageId;
    QString filename , newFileName;
    QFile file;
    bool is_Ubegin = false;
    bool is_Dbegin = false;

    // 最多等待30s
    while (m_socket->waitForReadyRead(30000)) {
        qDebug() << "接收到数据";
        QByteArray byteArray = m_socket->readAll();
        if (byteArray.isEmpty()) {
            qDebug() << "empty()";
            continue;
        }
        if (byteArray[0] == 'U' && !is_Ubegin) {
            // 上传文件
            QDataStream stream(&byteArray, QIODevice::ReadOnly);
            is_Ubegin = true;
            byteArray.remove(0, 1); // 去除开头
            stream >> totalSize; // 读取文件大小
            QByteArray nameByte;
            stream >> nameByte; // 读取文件名数据
            filename = QString::fromUtf8(nameByte);
            stream >> from;
            stream >> to;
            stream >> messageId; // 消息ID
            newFileName = dir.path() + "/file/" + getRandomString(7) + "_" + filename;
            file.setFileName(newFileName);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "open file fail : " << file.fileName();
                return;
            } else {
                qDebug() << "开始接收" << file.fileName() << "文件";
            }
            // 去除已读取部分
            byteArray.remove(0, sizeof(qint64) * 4 + sizeof(qint32) + filename.length());
            qDebug() << "将要去除的数据包的长度(开头不算):" << sizeof(qint64) + sizeof(qint32) + filename.length();
            file.write(byteArray);
            recvSize += byteArray.size();
            qDebug() << recvSize << "   " << totalSize;
            if (recvSize >= totalSize) {
                break;
            }
        } else if (is_Ubegin) {
            // 处理上传文件的数据包接收
            file.write(byteArray);
            recvSize += byteArray.size();
            qDebug() << recvSize << "   " << totalSize;
            if (recvSize >= totalSize) {
                break;
            }
        } else if (byteArray[0] == 'D' && !is_Dbegin) {
            // 下载文件
            is_Dbegin = true;
            byteArray.remove(0, 1);
            QDataStream stream(byteArray);
            qint64 to, from, messageId;
            stream >> messageId >> from >> to;
            qDebug() << " " << messageId << " " << from << " " << to;
            QVariantMap vmap = DBManager::singleTon().queryFiles(messageId, from, to);
            QString filename = vmap.value("filename").toString();
            qDebug() << "filename:" << filename;
            parseDownloadFile(filename);
            break;
        } else {
            // 错误
            qDebug() << "have a undefined error!";
            return;
        }
    }
    if (is_Ubegin) {

        if (recvSize < totalSize) {

            file.remove(); // 删除不完整的文件
            // qDebug() << "文件接收不完整，已删除";
        } else {

            qDebug() << "文件接收成功" << file.fileName();
            // 通知消息服务器进行转发

            double kilobytes = totalSize / 1024.0;
            double megabytes = totalSize / 1048576.0;

            QString filesize;
            if (totalSize >= 1048576) {
                filesize = QString::number(megabytes, 'f', 2) + " MB";
            } else if (totalSize >= 1024) {
                filesize = QString::number(kilobytes, 'f', 2) + " KB";
            } else {
                filesize = QString::number(totalSize) + " B";
            }

            TcpServer::singleTon().transferFile(from, to, newFileName, filesize, messageId);
        }

        file.close();
    }
}

// 进行下载文件的函数
/// 依次写入开头
/// 文件名字
/// 文件大小 8个字节
/// 文件包数据
void FileClientHandler::parseDownloadFile(const QString& filename)
{
    QEventLoop loop;
    if (filename.isEmpty()) {
        return;
    }
    connect(m_socket,&QTcpSocket::bytesWritten,this,&FileClientHandler::handleBytesWritten);
    qDebug() << "客户端想要下载" << filename;
    QFileInfo fileInfo(filename);
    QString fname = fileInfo.fileName();
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray uData = QByteArray("D");
        m_socket->write(uData);
        qint64 fileOffset = 0;
        qint64 fileSize = file.size(); // 文件大小
        toWrite = fileSize;
        QByteArray infoBytes;
        QDataStream stream(&infoBytes, QIODevice::WriteOnly);
        QByteArray namebytes = fname.toUtf8();
        stream << namebytes; // 写入文件名数据
        stream << fileSize; // 写入文件大小
        m_socket->write(infoBytes);
        while (fileOffset < fileSize) {
            file.seek(fileOffset);
            QByteArray byteArray = file.read(maxBlock);
            m_socket->write(byteArray);
            fileOffset += byteArray.size();
        }
    }else {
        qDebug() << "文件路径错误" << filename;
    }
    loop.exec();   //发送时需要开启事件循环
    file.close();
}


void FileClientHandler::handleBytesWritten(qint64 size)
{
    haveWritten += size;
    qDebug() << "成功写入:" << haveWritten;
    if (haveWritten == toWrite) {
        qDebug() <<"文件已经成功传送到客户端:" << toWrite;

    }
}


FileClientHandler::~FileClientHandler()
{
    if (m_socket != nullptr) {
        delete m_socket;
        m_socket = nullptr;
    }
}

