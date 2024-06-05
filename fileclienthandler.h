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
private:
    qintptr socketDescriptor;
    bool is_success = false;
    bool is_begin = false;
    QTcpSocket * m_socket = nullptr;

};

#endif // CLIENTHANDLER_H
