#ifndef RPC_PROTOCOL_H
#define RPC_PROTOCOL_H

#include <cstdint>
#include <string>
#include <cstring>
#include <arpa/inet.h>

#pragma pack(1)
struct RpcHeader {
    uint16_t magic;      // 魔数
    uint8_t  version;    // 版本号
    uint8_t  msg_type;   // 消息类型
    uint32_t msg_id;     // 消息ID
    uint32_t body_len;   // 消息体长度
};
#pragma pack()

char* serialize_message(const RpcHeader& header, const std::string& body, size_t& out_len);
bool deserialize_message(const char* data, size_t len, RpcHeader& header, std::string& body);

#endif // RPC_PROTOCOL_H