#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QByteArray>

// 只负责字节收发：
//  - 发：JSON 末尾补 '\0'（服务端按 '\0' 拆包）
//  - 收：服务端下发裸 JSON，没有分隔符，需要按花括号配平自己拆包
class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);

    // 异步连接，结果通过 connected / connectFailed 信号返回
    void connectTo(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // 发送一条 JSON（自动补 '\0'），未连接时返回 false
    bool sendFrame(const QJsonObject &obj);

signals:
    void connected();
    void disconnected();
    void connectFailed(const QString &reason);

    // 每收到一个完整的 JSON 对象就发一次
    void frameReady(const QJsonObject &frame);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    // 从接收缓冲区里切出所有完整的 JSON 对象
    void parseBuffer();

    QTcpSocket *_socket;
    QByteArray _buf;
};

#endif // TCPCLIENT_H
