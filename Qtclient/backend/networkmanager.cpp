#include "networkmanager.h"
#include <QJsonDocument>
#include "../utils/logger.h"
#include <QUrlQuery>
#include <QNetworkProxy>
NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverUrl("http://127.0.0.1:8080")
    , m_useProxy(false)
{
    setUseProxy(m_useProxy);

    // 连接WebSocket信号
    connect(&m_webSocket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &NetworkManager::onTextMessageReceived);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &NetworkManager::onError);
    
    // 连接QNetworkAccessManager的finished信号到统一处理函数
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &NetworkManager::onNetworkReplyFinished);
    
    LOG_INFO("NetworkManager initialized");
}

bool NetworkManager::useProxy() const
{
    return m_useProxy;
}

void NetworkManager::setUseProxy(bool useProxy)
{
    // 如果值没有变化，就什么也不做
    if (m_useProxy == useProxy)
        return;

    m_useProxy = useProxy;

    // 立即应用新的代理设置
    QNetworkProxy proxy;
    if (m_useProxy) {
        // 勾选：使用系统代理
        LOG_DEBUG("Setting proxy to 'DefaultProxy'");
        proxy.setType(QNetworkProxy::DefaultProxy);
    } else {
        // 未勾选：不使用任何代理
        LOG_DEBUG("Setting proxy to 'NoProxy'");
        proxy.setType(QNetworkProxy::NoProxy);
    }

    // 将设置应用到 HTTP 和 WebSocket
    m_networkManager->setProxy(proxy);
    m_webSocket.setProxy(proxy);

    // 发出信号，通知 QML 属性已更新
    emit useProxyChanged(m_useProxy);
}

void NetworkManager::disconnectFromServer()
{
    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        LOG_DEBUG("Disconnecting from server");
        m_webSocket.abort(); // 强制重置
    }
}

NetworkManager::~NetworkManager()
{
    LOG_INFO("NetworkManager destroyed");
    m_webSocket.close();
}

void NetworkManager::connectToServer(const QString &url)
{
    LOG_DEBUG("Disconnecting from server");
    disconnectFromServer();

    m_serverUrl = url;
    LOG_DEBUG("connectToServer called with url: %1", url);
}

void NetworkManager::sendLoginRequest(const QString &username, const QString &password)
{
    LOG_INFO("Sending login request for user: %1", username);
    
    // 构建登录请求
    QNetworkRequest request;
    QUrl url(m_serverUrl + "/login");
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 准备POST数据
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    QJsonDocument doc(json);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    LOG_DEBUG("Login request URL: %1", request.url().toString());
    LOG_DEBUG("Login request body: %1", QString(postData));
    
    // 发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, postData);
    reply->setProperty("username", username); // 保存用户名以便在响应中使用
    
    // 注意：不需要手动连接信号和槽，因为我们已经连接了QNetworkAccessManager的finished信号
    // 到统一处理函数onNetworkReplyFinished
}

void NetworkManager::sendRegisterRequest(const QString &username, const QString &password, const QString &email)
{
    LOG_INFO("Sending register request for user: %1", username);
    
    // 构建注册请求
    QNetworkRequest request;
    QUrl url(m_serverUrl + "/register");
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 准备POST数据
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    json["email"] = email;  // 添加邮箱字段
    QJsonDocument doc(json);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    LOG_DEBUG("Register request URL: %1", request.url().toString());
    LOG_DEBUG("Register request body: %1", QString(postData));
    
    // 发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, postData);
    reply->setProperty("username", username); // 保存用户名以便在响应中使用
    
    // 注意：不需要手动连接信号和槽，因为我们已经连接了QNetworkAccessManager的finished信号
    // 到统一处理函数onNetworkReplyFinished
}

// 新增的统一处理函数
void NetworkManager::onNetworkReplyFinished(QNetworkReply *reply)
{
    if (!reply) {
        LOG_ERROR("No reply object provided");
        return;
    }
    
    QUrl requestUrl = reply->request().url();
    QString endpoint = requestUrl.path();
    QString username = reply->property("username").toString();
    
    LOG_DEBUG("HTTP response received for endpoint: %1, user: %2, error: %3", 
              endpoint, 
              username, 
              QString::number(reply->error()));
    
    // 检查是否有网络错误
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR("Network error occurred: %1", reply->errorString());
        // 根据端点发送相应的错误信号
        if (endpoint == "/login") {
            emit loginResponseReceived(false, "网络错误: " + reply->errorString());
        } else if (endpoint == "/register") {
            emit registerResponseReceived(false, "网络错误: " + reply->errorString());
        }
        reply->deleteLater();
        return;
    }
    
    // 根据请求的端点分发到相应的处理函数
    if (endpoint == "/login") {
        onLoginRequestFinished(reply);
    } else if (endpoint == "/register") {
        onRegisterRequestFinished(reply);
    } else {
        LOG_WARN("Unknown endpoint: %1", endpoint);
        reply->deleteLater();
    }
}

void NetworkManager::onLoginRequestFinished(QNetworkReply *reply)
{
    if (!reply) {
        LOG_ERROR("No reply object provided");
        return;
    }
    
    // 获取请求URL以区分登录和注册请求
    QUrl requestUrl = reply->request().url();
    QString endpoint = requestUrl.path();
    
    LOG_DEBUG("HTTP response received for endpoint: %1, user: %2, error: %3", 
              endpoint, 
              reply->property("username").toString(), 
              QString::number(reply->error()));
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        LOG_DEBUG("HTTP response body: %1", QString(response));
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            LOG_DEBUG("Parsed JSON object: %1", QString(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
            
            if (obj.contains("type")) {
                QString type = obj["type"].toString();
                
                if (type == "login_success") {
                    // 登录成功，保存令牌
                    m_token = obj["token"].toString();
                    LOG_INFO("Login successful, token: %1", m_token);
                    
                    // 建立WebSocket连接
                    establishWebSocketConnection();

                } else if (type == "register_success") {
                    // 注册成功响应，应该由onRegisterRequestFinished处理
                    LOG_DEBUG("Received register success response in login handler, should be handled by onRegisterRequestFinished");
                    // 直接转发给注册处理函数
                    onRegisterRequestFinished(reply);
                    reply->deleteLater();  // 确保在这里释放reply对象
                    return;  // 立即返回，避免继续执行下面的代码
                } else if (type == "register_failed") {
                    // 注册失败响应，应该由onRegisterRequestFinished处理
                    LOG_DEBUG("Received register failed response in login handler, should be handled by onRegisterRequestFinished");
                    // 直接转发给注册处理函数
                    onRegisterRequestFinished(reply);
                    reply->deleteLater();  // 确保在这里释放reply对象
                    return;  // 立即返回，避免继续执行下面的代码
                } else {
                    // 登录失败
                    QString message = obj.contains("message") ? obj["message"].toString() : "登录失败";
                    LOG_WARN("Login failed, message: %1", message);
                    loginResponseReceived(false, message);
                }
            } else {
                // 未知响应类型，假设为登录响应
                QString message = obj.contains("message") ? obj["message"].toString() : "登录失败";
                LOG_WARN("Unknown response type, assuming login response, message: %1", message);
                loginResponseReceived(false, message);
            }
        } else {
            LOG_ERROR("JSON parse error: %1", parseError.errorString());
            loginResponseReceived(false, "服务器响应格式错误");
        }
    } else {
        LOG_ERROR("Network error: %1", reply->errorString());
        loginResponseReceived(false, "网络错误: " + reply->errorString());
    }
    
    reply->deleteLater();
}

void NetworkManager::onRegisterRequestFinished(QNetworkReply *reply)
{
    if (!reply) {
        LOG_ERROR("No reply object provided");
        return;
    }
    
    LOG_DEBUG("HTTP register response received for user: %1, error: %2", 
              reply->property("username").toString(), 
              QString::number(reply->error()));
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        LOG_DEBUG("HTTP register response body: %1", QString(response));
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            LOG_DEBUG("Parsed JSON object: %1", QString(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
            
            if (obj.contains("type") && obj["type"].toString() == "register_success") {
                // 注册成功
                QString message = obj.contains("message") ? obj["message"].toString() : "注册成功";
                LOG_INFO("Register successful, message: %1", message);
                registerResponseReceived(true, message);
            } else {
                // 注册失败
                QString message = obj.contains("message") ? obj["message"].toString() : "注册失败";
                LOG_WARN("Register failed, message: %1", message);
                registerResponseReceived(false, message);
            }
        } else {
            LOG_ERROR("JSON parse error: %1", parseError.errorString());
            registerResponseReceived(false, "服务器响应格式错误");
        }
    } else {
        LOG_ERROR("Network error: %1", reply->errorString());
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
    
    LOG_INFO("Establishing WebSocket connection to: %1", url.toString());
    
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
        LOG_DEBUG("Message sent: %1", QString(doc.toJson(QJsonDocument::Compact)));
    } else {
        LOG_WARN("WebSocket is not connected. Current state: %1", QString::number(m_webSocket.state()));
    }
}

void NetworkManager::onConnected()
{
    LOG_INFO("Connected to server");
    emit connectionStateChanged(true);

    // 在这里通知 QML 登录已完成，可以导航
    LOG_DEBUG("WebSocket connected, emitting loginResponseReceived(true)");
    emit loginResponseReceived(true, "登录成功");
}

void NetworkManager::onDisconnected()
{
    LOG_INFO("Disconnected from server");
    emit connectionStateChanged(false);
}

void NetworkManager::onTextMessageReceived(const QString &message)
{
    LOG_DEBUG("Received JSON message: %1", message);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        emit messageReceived(obj);
    } else {
        LOG_ERROR("Failed to parse JSON message: %1", parseError.errorString());
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket.errorString();
    LOG_ERROR("WebSocket error: %1", errorString);
    emit errorOccurred(errorString);
}
