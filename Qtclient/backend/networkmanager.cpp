#include "networkmanager.h"
#include <QJsonDocument>
#include <QDebug>
#include <QUrlQuery>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(&m_webSocket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &NetworkManager::onTextMessageReceived);
    connect(&m_webSocket, static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),
            this, &NetworkManager::onError);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &NetworkManager::onLoginRequestFinished);
}

NetworkManager::~NetworkManager()
{
    m_webSocket.close();
}

void NetworkManager::connectToServer(const QString &url)
{
    m_serverUrl = url;
    // 注意：现在我们不在这里直接连接WebSocket，而是在登录成功后再连接
    // 但我们需要保留此方法以保持API兼容性
}

void NetworkManager::sendLoginRequest(const QString &username, const QString &password)
{
    // 发送HTTP POST请求到/login端点
    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrl + "/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QByteArray postData;
    postData.append("username=" + username.toUtf8() + "&password=" + password.toUtf8());
    
    QNetworkReply *reply = m_networkManager->post(request, postData);
    // 不需要额外连接错误信号，因为QNetworkAccessManager::finished信号会处理所有情况
    // 包括错误情况
    
    // 存储reply对象以便在调试时识别
    reply->setProperty("username", username);
}

void NetworkManager::sendRegisterRequest(const QString &username, const QString &password)
{
    // 发送HTTP POST请求到/register端点
    QNetworkRequest request;
    request.setUrl(QUrl(m_serverUrl + "/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QByteArray postData;
    postData.append("username=" + username.toUtf8() + "&password=" + password.toUtf8());
    
    QNetworkReply *reply = m_networkManager->post(request, postData);
    // 存储reply对象以便在调试时识别
    reply->setProperty("username", username);
}

// 修改函数签名以接收QNetworkReply*参数
void NetworkManager::onLoginRequestFinished(QNetworkReply *reply)
{
    if (!reply) {
        qDebug() << "No reply object provided";
        return;
    }
    
    qDebug() << "HTTP response received for user:" << reply->property("username").toString() 
             << ", error:" << reply->error();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "HTTP response body:" << response;
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            qDebug() << "Parsed JSON object:" << obj;
            
            if (obj.contains("type") && obj["type"].toString() == "login_success") {
                // 登录成功，保存令牌
                m_token = obj["token"].toString();
                qDebug() << "Login successful, token:" << m_token;
                
                // 建立WebSocket连接
                establishWebSocketConnection();
                
                // 发送成功信号
                qDebug() << "Emitting loginResponseReceived(true, \"登录成功\")";
                loginResponseReceived(true, "登录成功");
            } else if (obj.contains("type") && obj["type"].toString() == "register_success") {
                // 注册成功响应，不应该在这里处理
                qDebug() << "Received register success response in login handler, ignoring";
                // 可以选择发送一个特殊的消息或者忽略
            } else {
                // 登录失败
                QString message = obj.contains("message") ? obj["message"].toString() : "登录失败";
                qDebug() << "Login failed, message:" << message;
                qDebug() << "Emitting loginResponseReceived(false, " << message << ")";
                loginResponseReceived(false, message);
            }
        } else {
            qDebug() << "JSON parse error:" << parseError.errorString();
            loginResponseReceived(false, "服务器响应格式错误");
        }
    } else {
        qDebug() << "Network error:" << reply->errorString();
        loginResponseReceived(false, "网络错误: " + reply->errorString());
    }
    
    reply->deleteLater();
}

void NetworkManager::onRegisterRequestFinished(QNetworkReply *reply)
{
    if (!reply) {
        qDebug() << "No reply object provided";
        return;
    }
    
    qDebug() << "HTTP register response received for user:" << reply->property("username").toString() 
             << ", error:" << reply->error();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "HTTP register response body:" << response;
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            qDebug() << "Parsed JSON object:" << obj;
            
            if (obj.contains("type") && obj["type"].toString() == "register_success") {
                // 注册成功
                QString message = obj.contains("message") ? obj["message"].toString() : "注册成功";
                qDebug() << "Register successful, message:" << message;
                qDebug() << "Emitting registerResponseReceived(true, " << message << ")";
                registerResponseReceived(true, message);
            } else {
                // 注册失败
                QString message = obj.contains("message") ? obj["message"].toString() : "注册失败";
                qDebug() << "Register failed, message:" << message;
                qDebug() << "Emitting registerResponseReceived(false, " << message << ")";
                registerResponseReceived(false, message);
            }
        } else {
            qDebug() << "JSON parse error:" << parseError.errorString();
            registerResponseReceived(false, "服务器响应格式错误");
        }
    } else {
        qDebug() << "Network error:" << reply->errorString();
        registerResponseReceived(false, "网络错误: " + reply->errorString());
    }
    
    reply->deleteLater();
}

void NetworkManager::establishWebSocketConnection()
{
    // 在WebSocket URL中添加令牌作为参数
    QUrl url(m_serverUrl);
    url.setScheme("ws"); // 确保使用WebSocket协议
    QUrlQuery query;
    query.addQueryItem("token", m_token);
    url.setQuery(query);
    
    qDebug() << "Establishing WebSocket connection to:" << url.toString();
    
    // 在连接前检查套接字状态
    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.close();
    }
    
    m_webSocket.open(url);
}

void NetworkManager::sendMessage(const QJsonObject &message)
{
    // 检查WebSocket连接状态
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(message);
        m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
    } else {
        qWarning() << "WebSocket is not connected. Current state:" << m_webSocket.state();
    }
}

void NetworkManager::onConnected()
{
    qDebug() << "Connected to server";
    emit connectionStateChanged(true);
}

void NetworkManager::onDisconnected()
{
    qDebug() << "Disconnected from server";
    emit connectionStateChanged(false);
}

void NetworkManager::onTextMessageReceived(const QString &message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON message:" << parseError.errorString();
        return;
    }

    QJsonObject response = doc.object();

    if (response.contains("type")) {
        QString type = response["type"].toString();

        if (type == "login_response") {
            bool success = response["success"].toBool();
            QString message = response["message"].toString();
            emit loginResponseReceived(success, message);
        } else {
            emit messageReceived(response);
        }
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket.errorString();
    qWarning() << "WebSocket error:" << errorString;
    emit errorOccurred(errorString);
}