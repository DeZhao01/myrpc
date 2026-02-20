// rpc_protocol.h
#ifndef RPC_PROTOCOL_H
#define RPC_PROTOCOL_H

#include <cstdint>
#include <string>
#include <cstring>
#include <arpa/inet.h>  // for htons, htonl, etc.

// 1. 协议头定义（使用1字节对齐，避免编译填充）
#pragma pack(1)
struct RpcHeader {
    uint16_t magic;      // 魔数
    uint8_t  version;    // 版本号
    uint8_t  msg_type;   // 消息类型
    uint32_t msg_id;     // 消息ID
    uint32_t body_len;   // 消息体长度
};
#pragma pack()

// 2. 序列化：将 RpcHeader 和 body 拼接到一块连续内存
// 返回值：动态分配的字节数组，调用者需使用 delete[] 释放
// 输出参数：out_len 返回总长度
char* serialize_message(const RpcHeader& header, const std::string& body, size_t& out_len) {
    // 计算总长度 = 头部长度 + 体长度
    size_t total_len = sizeof(RpcHeader) + body.size();
    char* buffer = new char[total_len];

    // 2.1 拷贝头部（注意转换到网络字节序）
    RpcHeader net_header;
    net_header.magic   = htons(header.magic);      // uint16_t 用 htons
    net_header.version = header.version;            // 单字节无需转换
    net_header.msg_type= header.msg_type;
    net_header.msg_id  = htonl(header.msg_id);     // uint32_t 用 htonl
    net_header.body_len= htonl(header.body_len);

    // 将网络序的头部拷贝到 buffer 前部
    memcpy(buffer, &net_header, sizeof(RpcHeader));

    // 2.2 拷贝消息体（字符串数据）
    if (!body.empty()) {
        memcpy(buffer + sizeof(RpcHeader), body.data(), body.size());
    }

    out_len = total_len;
    return buffer;
}

// 3. 反序列化：从接收到的字节流中解析出头部和body
// 输入：data 为接收到的字节流，len 为其长度
// 输出：成功返回 true，并将 header 和 body 填充好（已转换为主机序）
bool deserialize_message(const char* data, size_t len, RpcHeader& header, std::string& body) {
    if (len < sizeof(RpcHeader)) {
        return false;  // 数据不够一个头部
    }

    // 3.1 先拷贝头部原始数据（可能仍是网络序）
    RpcHeader net_header;
    memcpy(&net_header, data, sizeof(RpcHeader));

    // 3.2 转换为主机序
    header.magic    = ntohs(net_header.magic);
    header.version  = net_header.version;
    header.msg_type = net_header.msg_type;
    header.msg_id   = ntohl(net_header.msg_id);
    header.body_len = ntohl(net_header.body_len);

    // 3.3 校验魔数（可选）
    if (header.magic != 0x1234) {
        return false;  // 魔数不匹配，非法消息
    }

    // 3.4 检查长度是否足够包含完整消息体
    size_t expected_len = sizeof(RpcHeader) + header.body_len;
    if (len < expected_len) {
        return false;  // 数据不完整，需要继续接收
    }

    // 3.5 提取消息体
    if (header.body_len > 0) {
        body.assign(data + sizeof(RpcHeader), header.body_len);
    } else {
        body.clear();
    }

    return true;
}

#endif // RPC_PROTOCOL_H