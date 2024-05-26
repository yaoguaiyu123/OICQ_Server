#include "global.h"

//将QByteArray转换为QJsonValue的函数
QJsonValue byteArrayToJson(const QByteArray& jsonData) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isNull()) {
        if (doc.isObject()) {
            return doc.object();
        }else if (doc.isArray()){
            return doc.array();
        }else {
            qDebug() << "Json Document 无效";
        }
    } else {
        qDebug() << "无效的JSON:" << jsonData;
    }
    return QJsonValue();
}
