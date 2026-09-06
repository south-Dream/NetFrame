# NetFrame

NetFrame 是基于 C++20、Boost.Asio 和 Protobuf 实现的轻量 TCP 游戏服务端框架，主要用于长连接服务端中的连接管理、消息收发、协议解析、消息分发和心跳等基础流程。

## 已完成内容

* 基于 Boost.Asio 的异步 accept、read 和 write
* 多个 IoWorker，每个 Worker 持有独立的 `io_context` 和线程
* 一条连接对应一个 `Session`
* 8 字节 TCP 包头：消息 ID + 消息体长度
* Protobuf 消息序列化与反序列化
* `MessageManager` 根据消息 ID 创建对应消息
* `HandleManager` 将消息分发给对应 Handle
* Session 发送队列，保证同一连接上的消息顺序写出
* Ping / Pong 心跳检测和超时断开
* Worker 连接数量限制
* 基本的服务端退出处理

## 多 io_context 分片

NetFrame 使用多个 `io_context` 对网络处理进行分片。

程序中主要包含两类 `io_context`：

* `Server` 持有一个 `io_context`，负责监听端口、accept 新连接、处理退出信号以及维护全局 Tick。
* 每个 `IoWorker` 持有独立的 `io_context`，并启动一个线程，负责所属 Session 的网络收发和心跳检查。

整体结构大致如下：

```text
Server
  │
  │ accept
  ↓
IoWorkerManager
  │
  ├── IoWorker 0 ── Session...
  ├── IoWorker 1 ── Session...
  └── IoWorker 2 ── Session...
```

新连接到来前，`Server` 会通过 `IoWorkerManager` 选择一个还没有达到连接上限的 Worker，并创建对应 Session。

accept 成功后，Session 会被投递到对应 Worker，并在该 Worker 的 `io_context` 上开始读取数据。

Session 创建后不会在 Worker 之间迁移。

当前联调配置最多创建 3 个 Worker，每个 Worker 最多保存 10 条连接，主要用于快速验证：

* Worker 分配是否正常
* 单个 Worker 达到上限后的重新分配
* 所有 Worker 达到连接上限后的处理

这些参数只是当前联调用的小配置，不代表服务器的实际连接容量或性能上限，具体承载能力还需要通过后续压力测试确定。

### Session 线程归属

每个 Session 固定属于一个 IoWorker。

Session 内部的可变状态，例如：

* 发送队列
* Socket
* 心跳时间
* 连接状态

都只在所属 IoWorker 的线程中修改。

其他线程需要操作 Session 时，会通过：

```cpp
boost::asio::post(...)
```

将操作投递到 Session 所属的 `io_context`。

在这个约束下，发送队列等 Session 内部状态不需要额外加锁。

后续扩展时也需要保持这个规则，避免其他线程直接修改 Session 内部状态。

## 收包流程

TCP 本身是字节流，一次 read 不一定正好对应一条完整消息，因此当前协议使用固定包头 + 消息体长度的方式划分消息边界。

包头固定为 8 字节：

```text
4 bytes msgId
4 bytes bodyLen
```

两个整数都使用大端序。

收包流程：

```text
RecvHead()
    读取固定 8 字节
    ↓
解析 msgId 和 bodyLen
    ↓
检查 bodyLen 是否超过 1 MB
    ↓
RecvBody(msgId)
    按 bodyLen 读取完整消息体
    ↓
根据 msgId 创建对应 Protobuf 消息
    ↓
ParseFromArray
    ↓
HandleRecvMessage()
    ↓
交给 HandleManager
    ↓
再次 RecvHead()
```

固定长度包头负责确定消息边界，`async_read` 按指定长度读取完整包头和消息体。

当前以下情况会直接关闭连接：

* 消息体长度超过允许上限
* 消息 ID 未注册
* Protobuf 解析失败
* Socket 读写发生错误

当前单条消息体最大限制为 1 MB。

## 发包流程

业务代码调用：

```cpp
Session::Send(message)
```

后会依次处理：

1. 通过 `MessageManager` 查找消息类型对应的消息 ID。
2. 获取 Protobuf 序列化后的消息体长度。
3. 创建 8 字节消息包头。
4. 序列化 Protobuf 消息体。
5. 使用 `boost::asio::post` 投递到 Session 所属 Worker。
6. 将完整数据加入 Session 的发送队列。
7. 使用 `async_write` 发送队首消息。
8. 当前消息发送完成后继续发送下一条。

流程大致如下：

```text
Send(Message)
    ↓
Message Type → msgId
    ↓
Header + Protobuf Body
    ↓
post 到所属 Worker
    ↓
SendQueue
    ↓
async_write
    ↓
完成后继续下一条
```

发送队列主要用于：

* 避免同一个 Socket 同时进行多次写操作
* 保证消息按照进入发送队列的顺序写出

当前消息序列化发生在调用 `Send` 的线程，发送队列的修改和 Socket 写操作则统一在所属 IoWorker 中执行。

## 消息创建和分发

消息相关目前分为两个管理器：

* `MessageManager`
* `HandleManager`

### MessageManager

`MessageManager` 保存两组映射关系：

```text
msgId → 消息构造函数
消息 C++ 类型 → msgId
```

收包时：

```text
msgId
 ↓
MessageManager
 ↓
创建对应 Protobuf Message
```

发包时：

```text
Message Type
 ↓
MessageManager
 ↓
查找 msgId
```

只需要进行序列化或反序列化的消息，可以通过：

```cpp
REGISTER_M(...)
```

完成注册。

例如：

```cpp
REGISTER_M(1002, Pong);
```

表示：

```text
1002 ↔ Pong
```

### HandleManager

`HandleManager` 保存：

```text
消息 C++ 类型 → IHandle
```

Protobuf 消息解析完成后，会根据消息的实际类型找到对应 Handle。

`Handle<TMessage>` 负责将 Protobuf 基类转换成具体消息类型，再调用子类实现的：

```cpp
HandleMessage(...)
```

如果一条消息既需要注册消息 ID，又需要对应 Handle，可以使用：

```cpp
REGISTER_M_H(...)
```

例如：

```cpp
REGISTER_M_H(1001, Ping, HeartHandle);
REGISTER_M(1002, Pong);
```

服务端需要接收并处理 `Ping`，因此同时注册 `HeartHandle`。

`Pong` 当前只由服务端发送，所以只注册消息 ID。

### Handle 共享方式

当前每种消息类型只创建一个 Handle 实例，并由所有 IoWorker 共享。

因此同一个 Handle 实例可能同时被多个 Worker 线程调用。

Handle 本身不保存可变的运行时业务状态，主要负责：

```text
收到消息
 ↓
读取消息内容
 ↓
操作当前 Session
或者
调用其他业务模块
```

单连接状态放在 `Session` 中，并由所属 IoWorker 负责处理。

多个连接之间共享的状态，例如 Player、Room 或其他全局数据，需要放到独立业务模块中，由对应模块负责线程安全或串行调度。

比较耗时的逻辑也不能直接阻塞 Handle，否则会影响当前 IoWorker 上其他 Session 的网络处理。

没有为每个 IoWorker 单独创建一套 Handle，是因为这种方式只能隔离 Handle 自身的成员状态，并不能解决真正共享业务数据的并发访问问题。

## 心跳检查

连接建立后，Session 会记录最近一次收到 Ping 的时间，并将 Session ID 放入待检查的桶中。

每个 Worker 周期调用：

```cpp
SessionManager::Tick()
```

心跳检查流程：

```text
Session 进入检查桶
    ↓
到达对应 Tick
    ↓
检查最近一次 Ping 时间
    ↓
是否超过检查周期
    ↓
是 → missCount + 1
否 → 放入下一次检查桶
    ↓
missCount >= 3
    ↓
关闭连接
```

当前 Tick 间隔约为：

```text
50 ms
```

一个心跳检查周期为 100 Tick，约：

```text
5 秒
```

因此客户端正常情况下需要在一个检查周期内发送 Ping。

多个检查桶主要用于将 Session 的心跳检查分散到不同 Tick，避免每次 Tick 都遍历全部 Session。

当前实现只针对连接心跳需求，没有进一步抽象成通用 Timer 或任务调度模块。

## 关闭流程

### Session 异常

以下情况会触发 Session 关闭：

* Socket 读取失败
* Socket 写入失败
* 协议解析失败
* 心跳超时

Socket 关闭后，对应 Session 会在后续检查过程中从 `SessionManager` 中移除。

### 服务端退出

收到：

```text
SIGINT
SIGTERM
```

后，关闭流程大致为：

```text
停止 accept
 ↓
停止 Server io_context
 ↓
停止各个 IoWorker
 ↓
等待 Worker 线程退出
```

### 达到连接上限

如果当前所有 Worker 都达到连接上限，Server 仍然保持 accept。

新的 Socket 建立后会立即关闭，避免因为连接满载导致 accept 循环停止。

## 目录结构

```text
NetFrame/
├── include/NetFrame/
│   ├── Server.hpp
│   ├── net/
│   │   ├── IoWorker.hpp
│   │   └── IoWorkerManager.hpp
│   ├── session/
│   │   ├── Session.hpp
│   │   └── SessionManager.hpp
│   ├── message/
│   │   └── MessageManager.hpp
│   ├── handle/
│   │   ├── Handle.hpp
│   │   ├── HandleManager.hpp
│   │   └── HeartHandle.hpp
│   └── tool/
│       └── ...
│
├── src/
│   └── 对应实现
│
├── proto/
│   └── message/
│       ├── ping.proto
│       └── pong.proto
│
├── proto/generated/
│   └── 手动运行 generate_proto.bat 时生成，不提交仓库
│
├── Doc/
│   └── Protocol.md
│
├── Server.cpp
├── Main.cpp
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

客户端对接协议见：

[Doc/Protocol.md](Doc/Protocol.md)

## 构建和运行

开发环境：

* Visual Studio 2022
* CMake 3.20+
* C++20
* vcpkg

### 配置 vcpkg

安装 vcpkg 后，需要设置：

```text
VCPKG_ROOT
```

环境变量。

例如只设置当前 PowerShell 会话：

```powershell
$env:VCPKG_ROOT = "D:\vcpkg"
```

`CMakePresets.json` 会通过 `VCPKG_ROOT` 找到：

```text
scripts/buildsystems/vcpkg.cmake
```

随后 vcpkg 根据：

```text
vcpkg.json
```

准备项目需要的：

* Boost.Asio
* Protobuf

`CMakeLists.txt` 中不保存本机的绝对依赖路径。

机器相关的本地配置可以写入不提交 Git 的：

```text
CMakeUserPresets.json
```

### 构建

配置：

```powershell
cmake --preset x64
```

构建：

```powershell
cmake --build --preset x64-debug
```

运行：

```powershell
out/build/x64/Debug/NetFrame.exe
```

服务端当前默认监听：

```text
8888
```

端口。

## Protobuf 生成

服务端构建时，CMake 会自动调用 protoc，并在构建目录生成：

```text
.pb.h
.pb.cc
```

因此正常构建 NetFrame 时，不需要提前手动运行 Protobuf 生成脚本。

自动生成的文件位于构建目录，不会写入源码目录。

项目中同时保留：

```text
generate_proto.bat
```

用于需要单独生成协议文件的情况。

如果需要额外生成 Unity 使用的 C# 消息，可以传入输出目录：

```powershell
generate_proto.bat "C:\path\to\Generated"
```

脚本会依次尝试从：

1. `PATH`
2. 项目构建目录中的 vcpkg 安装结果
3. `VCPKG_ROOT`

查找 `protoc.exe`。

不传参数时生成 C++ 文件。

传入输出目录时同时生成 C# 文件。

## 当前实现取舍

当前版本优先保持网络层结构简单，没有提前加入过多业务模块。

主要取舍包括：

* 每个 IoWorker 使用一个线程
* 不为每条 Session 单独创建线程
* 暂时没有额外业务线程池
* Handle 直接运行在 IoWorker 线程
* 没有加入依赖注入
* 没有复杂的模块生命周期
* 消息注册使用模板和宏
* 心跳只针对当前 Ping / Pong 需求
* 没有单独实现通用 Timer 模块

当前先保证连接、收发、协议、分发和心跳等基础链路能够正常运行。

后续业务增加后，再根据实际需求拆分业务线程、生命周期和其他模块。

## 当前可以验证的内容

当前版本可以验证：

* 多个客户端同时连接 Server
* 客户端连接分配到不同 IoWorker
* Worker 达到连接上限后的重新分配
* 所有 Worker 满载后的连接处理
* 固定包头 + bodyLen 的 TCP 消息边界划分
* `async_read` 按指定长度读取完整包头和消息体
* Protobuf 消息根据消息 ID 创建
* 消息分发到对应 Handle
* 同一 Session 的多条消息通过发送队列顺序写出
* Ping / Pong 更新心跳状态
* 心跳超时后断开连接
* 服务端退出后客户端发现断线

## 当前限制

### IO 与耗时业务还没有拆分

Handle 当前直接运行在 IoWorker 线程。

如果 Handle 中执行：

* 数据库查询
* 大量计算
* 文件 IO
* 其他耗时逻辑

会阻塞当前 Worker 的事件循环，并影响该 Worker 上其他 Session 的网络处理。

业务复杂后需要增加业务线程池，或者将耗时逻辑交给独立模块执行。

### Handle 实例由多个 Worker 共享

同一种消息当前只有一个 Handle 实例，多个 Worker 可能并发调用。

因此 Handle 当前需要保持无状态。

如果后续需要让 Handle 保存运行时状态，需要重新考虑对应状态的线程归属和生命周期。

### 还没有进行完整压力测试

目前使用的：

```text
3 个 Worker
每个 Worker 10 条连接
```

只是联调配置。

还没有进行：

* 大连接数测试
* 高频消息收发测试
* 大消息测试
* 长时间稳定性测试

因此当前配置不能用于判断实际服务器承载能力。

### 协议版本处理较简单

目前客户端和服务端通过约定一致的：

```text
msgId
```

识别消息类型。

暂时没有单独的协议版本检查。

如果后续客户端和服务端存在多个版本，需要增加协议兼容和版本管理。

## 暂未覆盖

以下内容暂时不在当前版本范围内：

* 登录鉴权
* 数据库
* 玩家数据持久化
* 断线重连
* KCP
* WebSocket
* TLS
* 房间系统
* 游戏状态同步
* AOI
* 分布式部署

后续会优先通过实际游戏项目继续验证 NetFrame，再根据使用过程中出现的问题补充对应功能。
