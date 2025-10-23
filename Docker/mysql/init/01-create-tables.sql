-- 在每次启动时重新创建表结构
-- 注意：这会删除之前的所有数据

-- 删除已存在的表（如果存在）
DROP TABLE IF EXISTS user_friends;
DROP TABLE IF EXISTS user_status;
DROP TABLE IF EXISTS user_sessions;
DROP TABLE IF EXISTS messages;
DROP TABLE IF EXISTS users;

-- 创建用户表
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 创建消息表
CREATE TABLE messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id INT NOT NULL,
    receiver_id INT NOT NULL,
    content TEXT NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sender_id) REFERENCES users(id),
    FOREIGN KEY (receiver_id) REFERENCES users(id)
);

-- 创建用户会话表
CREATE TABLE user_sessions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    session_token VARCHAR(255) NOT NULL,
    expires_at TIMESTAMP NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- 创建用户状态表
CREATE TABLE user_status (
    user_id INT PRIMARY KEY,
    status ENUM('OFFLINE', 'ONLINE', 'AWAY', 'BUSY') NOT NULL DEFAULT 'OFFLINE',
    last_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    session_token VARCHAR(255),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 创建好友关系表
CREATE TABLE user_friends (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_friendship (user_id, friend_id)
);

-- 插入一些示例数据（可选）
INSERT INTO users (username, password, email) VALUES 
('alice', 'password123', 'alice@example.com'),
('bob', 'password456', 'bob@example.com');

-- 插入示例用户状态
INSERT INTO user_status (user_id, status, last_seen, session_token) VALUES
(1, 'ONLINE', NOW(), 'token1'),
(2, 'AWAY', DATE_SUB(NOW(), INTERVAL 5 MINUTE), 'token2');

-- 插入示例好友关系
INSERT INTO user_friends (user_id, friend_id) VALUES
(1, 2),
(2, 1);