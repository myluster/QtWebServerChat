# Docker 部署说明

## 概述

此目录包含了使用 Docker 部署 IM 系统所有服务的配置文件。该配置确保：

1. 所有微服务（GateServer、StatusServer、VarifyServer）共享同一个 MySQL 和 Redis 实例
2. 在每次 Docker Compose 启动时重新创建数据库表结构
3. 在 Docker Compose 停止时自动清除所有数据

## 服务架构

- **MySQL**: 数据库服务（端口 3306）
- **Redis**: 缓存服务（端口 6379）
- **GateServer**: 网关服务（端口 8080）
- **StatusServer**: 状态服务（端口 8081）
- **VarifyServer**: 验证服务（端口 8082）

## 使用方法

### 启动所有服务

```bash
docker-compose up --build
```

### 停止所有服务

```bash
docker-compose down
```

## 数据管理

### 数据初始化

MySQL 数据库会在每次启动时通过 `mysql/init/` 目录中的脚本重新初始化表结构。

### 数据清除

通过使用 tmpfs 卷，所有 MySQL 和 Redis 数据都会在容器停止时自动清除。

## 连接信息

### MySQL
- Host: localhost
- Port: 3306
- Database: im_database
- User: im_user
- Password: im_password

### Redis
- Host: localhost
- Port: 6379

## 开发注意事项

1. 修改数据库表结构时，请更新 `mysql/init/01-create-tables.sql` 文件
2. 每次运行 `docker-compose up` 都会重新创建表结构，之前的数据会被清除
3. 如果需要持久化数据，请修改 docker-compose.yml 中的卷配置