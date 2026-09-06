# NetFrame

NetFrame 是基于 C++20、Boost.Asio 和 Protobuf 实现的轻量 TCP 游戏服务端框架，用于验证长连接服务端的基础网络流程。

框架采用 Reactor 风格划分网络事件：`Server` 负责监听、接收连接和全局计时，多个 `IoWorker` 分别运行独立的 `io_context` 和线程，处理所属连接的异步收发与心跳检查。Unity 客户端用于验证连接、协议和消息收发链路。

## 已完成的内容

- 基于 Boost.Asio 的异步 accept、read 和 write
- Reactor 风格的事件循环划分
- 多个 IO Worker，每个 Worker 持有独立的 `io_context` 和线程
- 一条连接对应一个 `Session`
- 8 字节 TCP 包头：消息 ID + 消息体长度
- Protobuf 消息序列化与反序列化
- `MessageManager` 根据消息 ID 创建消息
- `HandleManager` 把消息交给对应的 Handle
- Session 发送队列，保证同一连接的消息按顺序写出
- Ping/Pong 心跳和超时断开
- 连接数量限制和基本的退出处理

## 一条消息怎样被处理

```text
客户端连接
    ↓
Server 接受连接并选择 IoWorker
    ↓
Session 读取 8 字节包头，再读取 Protobuf 消息体
    ↓
MessageManager 根据 msgId 创建并解析消息
    ↓
HandleManager 找到对应 Handle
    ↓
业务 Handle 处理消息，需要回复时调用 Session::Send
```

当前示例包含一组心跳消息：客户端发送 `Ping`，服务端的 `HeartHandle` 更新 Session 的心跳状态并回复 `Pong`。

## 目录

```text
NetFrame/
├── include/NetFrame/
│   ├── Server.hpp
│   ├── net/                 IoWorker 和 IoWorkerManager
│   ├── session/             Session 和 SessionManager
│   ├── message/             MessageManager
│   ├── handle/              Handle 基类、注册和心跳处理
│   └── tool/                日志与时间工具
├── src/                     对应实现
├── proto/message/           .proto 文件
├── proto/generated/         手动运行脚本时生成，不提交仓库
├── Doc/                     设计和协议说明
├── Server.cpp
└── Main.cpp
```

更具体的代码划分见 [Doc/README.md](Doc/README.md)，客户端对接格式见 [Doc/Protocol.md](Doc/Protocol.md)。

## 构建和运行

开发环境：

- Visual Studio 2022
- CMake + Ninja
- C++20
- Boost
- Protobuf

先安装 vcpkg，并设置 `VCPKG_ROOT` 环境变量。例如当前 PowerShell 会话中：

```powershell
$env:VCPKG_ROOT = "你的 vcpkg 目录"
```

随后配置并构建：

```powershell
cmake --preset x64
cmake --build --preset x64-debug
out/build/x64/Debug/NetFrame.exe
```

服务端默认监听 `8888` 端口。

`CMakeLists.txt` 不保存本机依赖路径。`CMakePresets.json` 通过 `VCPKG_ROOT` 查找 vcpkg；个人机器专用配置可以写在不会提交的 `CMakeUserPresets.json` 中。

CMake 会在构建目录自动生成服务端需要的 Protobuf C++ 文件，因此首次构建前不需要手动运行生成脚本。

如果还需要为 Unity 客户端生成 C# 消息，可以手动运行：

```powershell
generate_proto.bat "C:\path\to\UnityProject\Assets\NetFrameClient\Generated"
```

脚本会依次从 `PATH`、项目构建目录中的 vcpkg 安装结果和 `VCPKG_ROOT` 查找 `protoc.exe`。省略参数时只生成 C++ 文件；传入目录时会同时生成 C# 文件。

## Unity 联调

`NetFrame.Client` 是 Unity 2021.3 测试工程，用于验证：

1. 客户端能够连接服务端。
2. 两端按相同包头和 `.proto` 收发消息。
3. Ping/Pong 能持续工作并计算 RTT。
4. 服务端退出后客户端能够发现连接断开。

Unity 客户端只承担服务端可用性验证，不作为 NetFrame 仓库的主要展示内容。接入 3D 游戏项目后，可以通过运行截图或短视频展示实际联调结果。

## 当前不足

- 业务 Handle 仍运行在 IO worker 线程中，耗时逻辑会影响该 worker 上的网络处理。
- `HandleManager` 中每种消息只有一个 Handle 实例，多个 IO Worker 会共享调用，因此 Handle 需要保持无状态。
- 只做了 TCP，没有 KCP、WebSocket 和 TLS。
- 没有登录鉴权、数据库和断线重连。
- 消息 ID 需要两端保持一致，目前没有单独的协议版本检查。
- worker 数量和每个 worker 的连接上限还是练习时使用的小配置。
- 缺少完整的自动化压力测试。

以上内容属于当前版本的功能边界，可在后续迭代中按项目需求补充。
