# ============================================
# QtWebServerChat 开发环境启动脚本
# ============================================

set -e

echo "启动 QtWebServerChat 开发环境..."
echo "============================================"
echo ""

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 检查 Docker 是否运行，如果未运行则尝试启动
if ! docker info > /dev/null 2>&1; then
    echo "Docker 未运行，正在尝试启动 Docker..."
    
    # 尝试启动 Docker 服务
    if command -v systemctl &> /dev/null; then
        sudo systemctl start docker 2>/dev/null || {
            echo "无法自动启动 Docker，请手动启动 Docker 服务:"
            echo "sudo systemctl start docker"
            echo "或者运行: sudo dockerd"
            exit 1
        }
        
        # 等待 Docker 完全启动
        echo "等待 Docker 启动..."
        sleep 5
        
        # 再次检查 Docker 是否运行
        if ! docker info > /dev/null 2>&1; then
            echo "Docker 启动失败，请检查系统配置"
            exit 1
        fi
        
        echo "Docker 已成功启动!"
    else
        echo "系统不支持 systemctl，请手动启动 Docker 后重试"
        exit 1
    fi
fi

# 检查 docker compose 是否安装
if ! command -v docker-compose &> /dev/null; then
    echo "docker-compose 未安装"
    exit 1
fi

# 启动服务

cd "${PROJECT_ROOT}/Docker"
docker-compose -f docker-compose.dev.yml up -d

# 等待服务准备就绪
echo ""
echo "等待服务准备就绪..."
attempt=1
max_attempts=30

# 等待 MySQL 容器变为健康状态
echo "等待 MySQL 容器变为健康状态..."
while [ "$(docker inspect -f '{{.State.Health.Status}}' im_mysql_dev 2>/dev/null)" != "healthy" ]; do
    echo "  尝试 #$attempt: MySQL 容器尚未健康，等待中..."
    sleep 2
    ((attempt++))
    if [ $attempt -gt $max_attempts ]; then
        echo "MySQL 容器未能在规定时间内变为健康状态，请检查日志:"
        echo "docker logs im_mysql_dev"
        exit 1
    fi
done
echo "MySQL 容器已健康!"

# 等待 Redis 容器变为健康状态（如果配置了健康检查）
if docker inspect -f '{{.Config.Healthcheck}}' im_redis_dev &>/dev/null; then
    echo "等待 Redis 容器变为健康状态..."
    attempt=1
    while [ "$(docker inspect -f '{{.State.Health.Status}}' im_redis_dev 2>/dev/null)" != "healthy" ]; do
        echo "  尝试 #$attempt: Redis 容器尚未健康，等待中..."
        sleep 2
        ((attempt++))
        if [ $attempt -gt $max_attempts ]; then
            echo "Redis 容器未能在规定时间内变为健康状态，请检查日志:"
            echo "docker logs im_redis_dev"
            exit 1
        fi
    done
    echo "Redis 容器已健康!"
fi

# 检查服务状态
echo ""
echo "服务状态："
docker-compose -f docker-compose.dev.yml ps

# 显示访问信息
echo ""
echo "============================================"
echo "Docker 服务启动完成！"
echo "============================================"
echo ""
echo "服务访问信息："
echo "  MySQL:             localhost:3306"
echo "    - 用户名: im_user"
echo "    - 密码: password"
echo "    - 数据库: im_database"
echo ""
echo "  Redis:             localhost:6379"
echo ""
echo "命令："
echo "  查看日志:   docker-compose -f ${PROJECT_ROOT}/Docker/docker-compose.dev.yml logs -f [service]"
echo "  停止服务:   ${PROJECT_ROOT}/scripts/stop-dev.sh"
echo ""

# 启动微服务
echo "启动微服务..."
cd "${PROJECT_ROOT}/Services/build"

# 启动 GateServer 并将输出重定向到日志文件
if [ -f "./GateServer/GateServer" ]; then
    ./GateServer/GateServer 0.0.0.0 8080 > gate_server.log 2>&1 &
    echo "GateServer 已启动，日志已写入 gate_server.log"
else
    echo "警告: GateServer 可执行文件未找到"
fi

# 启动 StatusServer 并将输出重定向到日志文件
if [ -f "./StatusServer/StatusServer" ]; then
    ./StatusServer/StatusServer > status_server.log 2>&1 &
    echo "StatusServer 已启动，日志已写入 status_server.log"
else
    echo "警告: StatusServer 可执行文件未找到"
fi

# 启动 VarifyServer 并将输出重定向到日志文件
if [ -f "./VarifyServer/VarifyServer" ]; then
    ./VarifyServer/VarifyServer > varify_server.log 2>&1 &
    echo "VarifyServer 已启动，日志已写入 varify_server.log"
else
    echo "警告: VarifyServer 可执行文件未找到"
fi

echo ""
echo "============================================"
echo "所有服务已成功启动！"
echo "============================================"
echo ""
echo "微服务日志查看命令："
echo "  GateServer 日志:   tail -f ${PROJECT_ROOT}/Services/build/gate_server.log"
echo "  StatusServer 日志: tail -f ${PROJECT_ROOT}/Services/build/status_server.log"
echo "  VarifyServer 日志: tail -f ${PROJECT_ROOT}/Services/build/varify_server.log"
echo ""
echo "停止所有服务命令："
echo "  ${PROJECT_ROOT}/scripts/stop-dev.sh"
echo ""