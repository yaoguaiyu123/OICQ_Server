#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QRunnable>
#include <QTcpSocket>

//传输文件的网络套接字

class FileClientHandler : public QRunnable
{
public:
    FileClientHandler(qintptr socketDescriptor);
    ~FileClientHandler();
    void run() override;
    void parseDownloadFile(const QString& filename);
public slots:
    void handleBytesWritten(qint64 size);
private:
    qintptr socketDescriptor;
    bool is_success = false;
    bool is_begin = false;
    QTcpSocket * m_socket = nullptr;

    // 发送
    qint64 haveWritten;
    qint64 toWrite;
};

#endif // CLIENTHANDLER_H
