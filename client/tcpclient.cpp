#include "tcpclient.h"

#include <QJsonDocument>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent),
      _socket(new QTcpSocket(this))
{
    connect(_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(_socket, &QAbstractSocket::errorOccurred, this, &TcpClient::onError);
}

void TcpClient::connectTo(const QString &host, quint16 port)
{
    // 重连前先清掉上一次的状态，避免把旧数据当成新消息
    if (_socket->state() != QAbstractSocket::UnconnectedState)
    {
        _socket->abort();
    }
    _buf.clear();
    _socket->connectToHost(host, port);
}

void TcpClient::disconnectFromServer()
{
    _socket->disconnectFromHost();
}

bool TcpClient::isConnected() const
{
    return _socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpClient::sendFrame(const QJsonObject &obj)
{
    if (!isConnected())
    {
        return false;
    }

    QByteArray frame = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    frame.append('\0'); // 服务端以 '\0' 作为一条消息的结束标记
    return _socket->write(frame) == frame.size();
}

void TcpClient::onConnected()
{
    emit connected();
}

void TcpClient::onDisconnected()
{
    _buf.clear();
    emit disconnected();
}

void TcpClient::onError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError)
    {
        return; // 服务端正常断开，交给 disconnected 信号处理，不算连接失败
    }
    emit connectFailed(_socket->errorString());
}

void TcpClient::onReadyRead()
{
    _buf.append(_socket->readAll());
    parseBuffer();
}

void TcpClient::parseBuffer()
{
    for (;;)
    {
        int depth = 0;
        int start = -1;
        int end = -1;
        bool inStr = false;
        bool escape = false;

        for (int i = 0; i < _buf.size(); ++i)
        {
            const char c = _buf.at(i);

            if (inStr)
            {
                // 字符串内部的花括号不算层级，还要处理 \" 之类的转义
                if (escape)
                {
                    escape = false;
                }
                else if (c == '\\')
                {
                    escape = true;
                }
                else if (c == '"')
                {
                    inStr = false;
                }
                continue;
            }

            if (c == '"')
            {
                inStr = true;
            }
            else if (c == '{')
            {
                if (depth == 0)
                {
                    start = i;
                }
                ++depth;
            }
            else if (c == '}')
            {
                if (depth > 0 && --depth == 0)
                {
                    end = i;
                    break;
                }
            }
        }

        if (end == -1)
        {
            break; // 还没收全，等下一次 readyRead
        }

        const QByteArray frame = _buf.mid(start, end - start + 1);
        _buf.remove(0, end + 1);

        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(frame, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
        {
            emit frameReady(doc.object());
        }
    }
}
