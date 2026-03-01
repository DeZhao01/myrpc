//
// Created by Lenovo on 2026/3/1.
//

#include "rpc_protocol.h"

char* serialize_message(const RpcHeader& header, const std::string& body, size_t& out_len) {
    size_t total_len = sizeof(RpcHeader) + body.size();
    char* buffer = new char[total_len];

    RpcHeader net_header;
    net_header.magic   = htons(header.magic);
    net_header.version = header.version;
    net_header.msg_type= header.msg_type;
    net_header.msg_id  = htonl(header.msg_id);
    net_header.body_len= htonl(header.body_len);

    memcpy(buffer, &net_header, sizeof(RpcHeader));
    if (!body.empty()) {
        memcpy(buffer + sizeof(RpcHeader), body.data(), body.size());
    }

    out_len = total_len;
    return buffer;
}

bool deserialize_message(const char* data, size_t len, RpcHeader& header, std::string& body) {
    if (len < sizeof(RpcHeader)) return false;

    RpcHeader net_header;
    memcpy(&net_header, data, sizeof(RpcHeader));

    header.magic    = ntohs(net_header.magic);
    header.version  = net_header.version;
    header.msg_type = net_header.msg_type;
    header.msg_id   = ntohl(net_header.msg_id);
    header.body_len = ntohl(net_header.body_len);

    if (header.magic != 0x1234) return false;

    size_t expected_len = sizeof(RpcHeader) + header.body_len;
    if (len < expected_len) return false;

    if (header.body_len > 0) {
        body.assign(data + sizeof(RpcHeader), header.body_len);
    } else {
        body.clear();
    }
    return true;
}