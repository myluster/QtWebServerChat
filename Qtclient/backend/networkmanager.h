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
    Q_INVOKABLE void sendRegisterRequest(const QString &username, const QString &password, const QString &email);
    Q_INVOKABLE void sendMessage(const QJsonObject &message);
    Q_INVOKABLE void disconnectFromServer();

signals:
    void connectionStateChanged(bool connected);
    void loginResponseReceived(bool success, const QString &message);
    void registerResponseReceived(bool success, const QString &message);
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onLoginRequestFinished(QNetworkReply *reply);
    void onRegisterRequestFinished(QNetworkReply *reply);
    void onNetworkReplyFinished(QNetworkReply *reply);  // 新增的统一处理函数

private:
    QWebSocket m_webSocket;
    QNetworkAccessManager *m_networkManager;
    QString m_token;  // 存储认证令牌
    QString m_serverUrl;  // 存储服务器URL

    void establishWebSocketConnection();
};

#endif // NETWORKMANAGER_H