#include "fileserver.h"
#include "fileclienthandler.h"
#include <QDebug>
#include <QTcpSocket>

FileServer::FileServer(QObject* parent)
    : QTcpServer(parent)
{
    startFileServer(QHostAddress::Any, 8081);
}

FileServer::~FileServer()
{
    QThreadPool::globalInstance()->waitForDone();
}

bool FileServer::startFileServer(const QHostAddress& address, quint16 port)
{
    qDebug() << tr("文件服务器开始在%1端口监听").arg(port);
    return listen(address, port);
}

// 接收
void FileServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "接收到新的文件上传请求";
    // QThreadPool默认自动删除QRunnable，在执行完run方法之后
    FileClientHandler *handler = new FileClientHandler(socketDescriptor);
    QThreadPool::globalInstance()->start(handler);
}
