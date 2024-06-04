#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QRunnable>
#include <QTcpSocket>

//传输文件的网络套接字

class FileClientHandler : public QRunnable
{
public:
    FileClientHandler(qintptr socketDescriptor);
    void run() override;
private:
    qintptr socketDescriptor;
    bool is_success = false;
    bool is_begin = false;

};

#endif // CLIENTHANDLER_H
