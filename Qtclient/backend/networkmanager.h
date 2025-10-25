#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "../utils/logger.h"

class NetworkManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool useProxy READ useProxy WRITE setUseProxy NOTIFY useProxyChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserIdChanged)

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    Q_INVOKABLE void connectToServer(const QString &url);
    Q_INVOKABLE void sendLoginRequest(const QString &username, const QString &password);
    Q_INVOKABLE void sendRegisterRequest(const QString &username, const QString &password, const QString &email);
    Q_INVOKABLE void sendMessage(const QJsonObject &message);
    Q_INVOKABLE void searchUser(const QString &query);
    Q_INVOKABLE void sendAddFriendRequest(int userId, int friendId);  // 添加好友请求
    Q_INVOKABLE void getFriendsList(int userId);  // 获取好友列表
    Q_INVOKABLE void getChatHistory(int userId, int friendId);  // 获取聊天历史记录
    Q_INVOKABLE void disconnectFromServer();

    bool useProxy() const;
    void setUseProxy(bool useProxy);

    QString currentUsername() const;
    QString currentUserId() const;

signals:
    void connectionStateChanged(bool connected);
    void loginResponseReceived(bool success, const QString &message);
    void registerResponseReceived(bool success, const QString &message);
    void messageReceived(const QJsonObject &message);
    void addFriendResponseReceived(bool success, const QString &message);  // 添加好友响应
    void friendsListReceived(const QJsonArray &friends);  // 好友列表响应
    void chatHistoryReceived(const QJsonArray &messages);  // 聊天历史记录响应
    void searchUserResponseReceived(const QJsonArray &results);
    void errorOccurred(const QString &error);
    void useProxyChanged(bool useProxy);

    void currentUsernameChanged(QString currentUsername);
    void currentUserIdChanged(QString currentUserId);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onLoginRequestFinished(QNetworkReply *reply);
    void onRegisterRequestFinished(QNetworkReply *reply);
    void onNetworkReplyFinished(QNetworkReply *reply);  // 新增的统一处理函数

private:
    bool m_useProxy;
    QWebSocket m_webSocket;
    QNetworkAccessManager *m_networkManager;
    QString m_token;  // 存储认证令牌
    QString m_serverUrl;  // 存储服务器URL

    QString m_currentUsername;
    QString m_currentUserId;

    void establishWebSocketConnection();
    void sendGRPCRequest(const QString &method, const QJsonObject &requestData);  // 发送gRPC请求
};

#endif // NETWORKMANAGER_H
