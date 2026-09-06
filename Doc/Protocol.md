# NetFrame 客户端协议

## 消息帧

每条消息由 8 字节包头和 Protobuf 消息体组成：

```text
+----------------------+----------------------+-------------------+
| msgId: uint32 大端   | bodyLen: uint32 大端 | Protobuf body     |
| 4 字节               | 4 字节               | bodyLen 字节       |
+----------------------+----------------------+-------------------+
```

- `msgId`：消息编号，用来确定消息类型。
- `bodyLen`：后面 Protobuf 数据的字节数。
- `body`：对应消息序列化后的内容。
- `bodyLen` 最大为 1 MB，超过限制时接收方关闭连接。

TCP 接收端应先读满 8 字节包头，再根据 `bodyLen` 读满消息体。不能假设一次 Socket read 就能得到一条完整消息。

## 当前消息

| ID | 方向 | Protobuf 类型 | 用途 |
|---:|---|---|---|
| 1001 | 客户端 → 服务端 | `Ping` | 客户端心跳 |
| 1002 | 服务端 → 客户端 | `Pong` | 回复心跳并回显序号 |

消息 ID 目前分别写在服务端注册代码和客户端消息特性中。增加消息时必须保证两边使用相同 ID。

## Protobuf 文件

协议源文件位于：

```text
proto/message/ping.proto
proto/message/pong.proto
```

客户端和服务端必须从同一份文件生成代码，不要分别手写字段号。

`Ping`：

```proto
message Ping
{
    uint32 sequence = 1;
    uint32 clientSendTime = 2;
}
```

`Pong`：

```proto
message Pong
{
    uint32 sequence = 1;
    uint32 clientSendTime = 2;
    uint32 serverSendTime = 3;
}
```

- `sequence`：客户端递增的心跳序号，服务端原样返回。
- `clientSendTime`：客户端发送时的运行时间，返回后用于估算 RTT。
- `serverSendTime`：预留的服务端时间字段；当前示例值仍是 `1`，不能当作真实服务器时间使用。

## 心跳行为

客户端需要每个一段时间发送 Ping 来进行心跳检测行为，服务端收到 Ping 后：

1. 清空该 Session 的漏跳次数。
2. 更新最近心跳时间。
3. 回复 Pong。

服务端大约每 5 秒检查一次对应 Session。连续 3 次检查都没有新的心跳时关闭连接，所以客户端心跳间隔必须小于 5 秒。

## 异常处理

当前双方遇到下列情况会关闭连接：

- 消息体长度超过 1 MB。
- 消息 ID 没有注册。
- Protobuf 消息解析失败。
- Socket 读取或发送失败。
- 心跳超时。

当前协议没有版本号、校验和、压缩和加密，也没有在断开前发送单独的错误消息。
