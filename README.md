# 微信风格即时通讯系统（C++ 服务端 + Qt 客户端）

一个从零实现的教学级 IM 项目：muduo 网络库的多线程 TCP 服务端 + Qt 6 Widgets 桌面客户端，支持单聊、群聊、好友申请、离线消息、历史消息、备注与忘记密码等完整链路。

服务端通过 **Redis 发布订阅** 在多个实例之间路由消息，前面用 **nginx stream** 做四层负载均衡；MySQL 持久化用户、好友、群组和消息。

## 架构

```
Qt 客户端 ──TCP──> nginx stream :8010 ──轮询──> chat1 :6000 ─┐
                                                   chat2 :6000 ─┼─> MySQL（用户/好友/群/消息）
                                                                └─> Redis（跨实例消息路由、在线状态通道）
```

## 目录结构

```
.
├─ server/                  # C++ 服务端
│  ├─ include/              # 头文件（含公共协议 include/public.hpp）
│  ├─ src/server/           # 服务主逻辑：chatserver / chatservice / model / db / redis
│  ├─ thirdparty/json.hpp   # 单头文件 JSON 库
│  ├─ sql/schema.sql        # 完整建表语句（docker 首次启动自动执行）
│  ├─ deploy/nginx.conf     # 本地（非容器）nginx 配置
│  └─ Dockerfile
├─ client/                  # Qt 6 Widgets 客户端
│  ├─ *.cpp / *.h / *.ui    # 界面与业务代码
│  ├─ res/                  # 图标、头像等资源
│  ├─ wechat.pro            # qmake 工程文件
│  └─ package.ps1           # windeployqt 打包脚本
├─ deploy/                  # 容器版 nginx（Dockerfile + 配置）
└─ docker-compose.yml       # mysql + redis + 两个服务端实例 + nginx
```

## 快速开始（服务端，Docker）

```bash
docker compose up -d --build
```

启动后客户端连接 `127.0.0.1:8010` 即可。包含的服务：

| 服务 | 说明 |
|---|---|
| mysql:8.4 | 首次启动自动执行 `server/sql/schema.sql`，库名 `chat` |
| redis:7 | 服务端多实例之间的消息路由 |
| chat1 / chat2 | 同一镜像的两个 ChatServer 实例（容器内 6000 端口） |
| nginx | stream 四层代理，容器内监听 8000，宿主机映射到 8010 |

- 数据库 root 密码默认 `123456`，可用环境变量覆盖：`MYSQL_ROOT_PASSWORD=xxx docker compose up -d --build`
- 宿主机 8000 被占用时可改 `docker-compose.yml` 里的 `8010:8000` 映射
- 构建时 muduo 默认从 GitHub 拉取，国内网络可指定镜像源：
  `MUDUO_REPO=https://ghfast.top/https://github.com/chenshuo/muduo.git docker compose up -d --build`
- 停止：`docker compose down`；连数据一起清空：`docker compose down -v`

## 服务端本地开发（Linux / WSL）

依赖：`cmake`、`g++`、`libmuduo`、`libhiredis-dev`、`libmysqlclient-dev`。

```bash
cd server
cmake -S . -B build
cmake --build build -j
./bin/ChatServer 6000
```

服务端通过环境变量读取连接信息，未设置时使用本地默认值：

| 变量 | 默认值 |
|---|---|
| `DB_HOST` / `DB_USER` / `DB_PASS` / `DB_NAME` | `127.0.0.1` / `root` / `123456` / `chat` |
| `REDIS_HOST` / `REDIS_PORT` | `127.0.0.1` / `6379` |

## 客户端构建（Windows）

1. 用 Qt Creator 打开 `client/wechat.pro`（Qt 6.9.3 + MinGW 64-bit）
2. 以 Release 模式构建，产物在 `client/bin/wechat.exe`
3. 打包成免安装 zip：

```powershell
powershell -ExecutionPolicy Bypass -File .\client\package.ps1
# 输出：client/dist/wechat-windows-x64.zip
```

脚本会自动调用 `windeployqt` 拷贝 Qt 依赖（若 Qt 路径不同，用 `-QtBin` 指定 `windeployqt.exe` 所在目录）。

> 客户端默认连接 `127.0.0.1:8010`，连接远程服务端时修改 `client/chatservice.cpp` 中的 `_host` 与 `_port`（或本地做端口转发）。

## 技术栈

- 服务端：C++17、muduo、MySQL 8、Redis 7、nginx（stream）
- 客户端：C++17、Qt 6.9.3 Widgets（qmake + .ui 设计器）
- 部署：Docker / docker compose

## 协议

客户端与服务端共用同一套消息枚举，定义在 `server/include/public.hpp`（登录、注册、单聊、群聊、好友申请、历史消息、备注、改密等），报文为 JSON。

## 开发过程

按提交阶段归档的历史快照仓库：[the-story-of-wechat](https://github.com/Xiaoming0731/the-story-of-wechat)
