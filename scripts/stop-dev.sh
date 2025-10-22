# ============================================
# QtWebServerChat 开发环境停止脚本
# ============================================

echo "停止 QtWebServerChat 开发环境..."
echo "============================================"
echo ""

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "停止微服务..."

# 获取所有微服务的进程ID并终止它们
pkill -f GateServer
pkill -f StatusServer
pkill -f VarifyServer

echo "所有微服务已停止。"

# 停止 Docker 服务
echo "停止 Docker 服务..."
cd "${PROJECT_ROOT}/Docker"
docker-compose -f docker-compose.dev.yml down -v

echo ""
echo "============================================"
echo "所有服务已成功停止！"
echo "============================================"
echo ""