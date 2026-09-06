# NetFrame 代码设计说明

## 多 io_context 分片

NetFrame 基于 Boost.Asio 构建多 io_context 分片的异步 I/O 模型：

- `Server` 的 `io_context`：负责监听端口、accept 新连接、处理退出信号和增加全局帧计数。
- `IoWorker` 的 `io_context`：每个 worker 启动一个线程，负责分配给它的 Session 收发和心跳检查。

新连接到来前，`Server` 先通过 `IoWorkerManager` 选择一个还没达到连接上限的 worker，并创建 Session。accept 成功后，再把 Session 投递到对应 worker 上开始读取数据。

当前联调配置最多创建 3 个 Worker，每个 Worker 最多保存 10 条连接，用于快速验证 Worker 分配、连接达到上限后的处理等逻辑。以上数值仅为联调参数，不代表服务器的实际连接容量或性能上限，具体承载能力需要通过后续压力测试确定。

Session 创建后不会在 worker 之间移动。Session 的可变状态只在所属 IoWorker 的线程中访问，其他线程对 Session 的操作通过 boost::asio::post 投递到所属 io_context。因此发送队列等 Session 内部状态不需要额外加锁。

## 收包流程

TCP 是字节流，一次 read 不等于一条完整消息，所以 Session 分两步读取：

```text
RecvHead()
    读取固定 8 字节
    得到 msgId 和 bodyLen
    检查 bodyLen 是否超过 1 MB
        ↓
RecvBody(msgId)
    按 bodyLen 读取完整消息体
    根据 msgId 创建 Protobuf 消息
    ParseFromArray
        ↓
HandleRecvMessage()
    交给 HandleManager
        ↓
再次 RecvHead()
```

包头中的两个整数都按大端序读写。消息体长度异常、消息 ID 没有注册或 Protobuf 解析失败时，当前处理方式都是直接关闭连接。

## 发包流程

业务代码调用 `Session::Send(message)` 后会：

1. 通过 `MessageManager` 查询消息类型对应的 ID。
2. 计算 Protobuf 消息体长度。
3. 写入 8 字节包头并序列化消息体。
4. 使用 `boost::asio::post` 把结果送到 Session 所属 worker。
5. 放入 Session 的发送队列。
6. 使用 `async_write` 逐条发送，上一条完成后再发送下一条。

发送队列的作用是避免同一 Socket 同时进行多次写操作，也保证消息按入队顺序发出。

## 消息创建和分发

消息处理分成两个小管理器：

### MessageManager

保存两组关系：

- `msgId → 消息构造函数`：收包时创建具体 Protobuf 消息。
- `消息 C++ 类型 → msgId`：发包时写入消息 ID。

`REGISTER_M` 用于注册只需要序列化或反序列化的消息。

### HandleManager

保存 `消息 C++ 类型 → IHandle`。收到并解析完消息后，根据消息实际类型找到处理器。

`Handle<TMessage>` 只负责把 Protobuf 基类引用转成具体消息类型，再调用子类的 `HandleMessage`。`REGISTER_M_H` 会同时注册消息和 Handle。

`HandleManager` 中每种消息类型只创建一个 Handle 实例，并由所有 IO Worker 共享调用，不为每个 Worker 单独创建 Handle。Handle 应保持无状态，只读取消息、操作当前 Session 或调用其他业务模块：

- 单连接状态放在 `Session` 中，并在所属 IO Worker 上处理。
- 多连接共享状态放在独立的业务管理器中，由该模块负责线程安全或串行调度。
- 耗时任务不能直接阻塞 Handle，否则会阻塞当前 IO Worker 的事件循环。

为每个 IO Worker 创建一套 Handle 只能隔离各 Worker 内的成员状态，同时会把业务状态按 Worker 分片，不能代替共享状态的并发控制。因此当前实现保留全局无状态 Handle。

当前例子中：

```cpp
REGISTER_M(1002, Pong);
REGISTER_M_H(1001, Ping, HeartHandle);
```

服务端需要接收 `Ping`，所以它带有 `HeartHandle`；`Pong` 只由服务端发出，因此只注册消息 ID。

## 心跳检查

连接建立时，Session 会记录当前时间，并把 Session ID 放到一个待检查的桶中。每个 worker 按帧调用 `SessionManager::Tick()`：

- 到达 Session 所在的检查桶时，比较当前时间和上次收到 Ping 的时间。
- 超过一个检查周期就增加一次漏跳次数。
- 连续达到 3 次后关闭并移除 Session。
- 未超时的 Session 放入下一次要检查的桶。

默认一帧约 50 ms，一个检查周期 100 帧，因此客户端应当在 5 秒以内发送一次 Ping。测试客户端使用 2 秒间隔。

这里使用多个桶，是为了不用每一帧遍历全部 Session。当前实现仍然比较简单，没有单独做通用定时器模块。

## 关闭流程

- Session 读写出错或心跳超时时，关闭对应 Socket，并在后续检查中从 SessionManager 移除。
- 收到 `SIGINT` 或 `SIGTERM` 时，Server 停止 accept 和主 `io_context`，然后关闭并等待各个 worker 线程。
- 达到连接上限时，Server 仍然继续 accept，但会立即关闭新 Socket，避免接受循环停止。

## 当前实现取舍

- 单个 worker 只有一个线程，没有给每条 Session 再创建线程。
- 没有额外的业务线程池，Handle 直接在 IO worker 上运行。
- 没有做依赖注入或复杂的模块生命周期。
- 消息注册使用模板和宏，数量少时比较直观。
- 心跳只处理当前 Ping/Pong 需求，没有做通用任务调度器。

这些实现控制了当前代码量。业务逻辑增加后，需要将耗时任务移出 IO 线程，并补充相应的模块生命周期和测试。

## 目前能验证什么

- 多个客户端可以被分配到 worker。
- 通过固定长度包头 + 消息体长度字段完成 TCP 消息边界划分，并使用 async_read 按指定长度读取完整包头和消息体。
- Protobuf 消息能够按 ID 创建并分发。
- 同一 Session 的多条发送通过队列串行执行。
- Unity 客户端可以持续发送 Ping、接收 Pong，并在服务端退出后发现断线。

当前版本用于验证以上链路。鉴权、持久化、分布式部署和游戏状态同步不在现有功能范围内。
