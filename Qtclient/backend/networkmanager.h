#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    Q_INVOKABLE void connectToServer(const QString &url);
    Q_INVOKABLE void sendLoginRequest(const QString &username, const QString &password);
    Q_INVOKABLE void sendMessage(const QJsonObject &message);

signals:
    void connectionStateChanged(bool connected);
    void loginResponseReceived(bool success, const QString &message);
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    
    // 修改函数签名以接收QNetworkReply*参数
    void onLoginRequestFinished(QNetworkReply *reply);

private:
    QWebSocket m_webSocket;
    QNetworkAccessManager *m_networkManager;
    QString m_token;  // 存储认证令牌
    QString m_serverUrl;  // 存储服务器URL

    void establishWebSocketConnection();
};

#endif // NETWORKMANAGER_H