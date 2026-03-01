//
// Created by Lenovo on 2026/3/1.
//

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "rpc_protocol.h"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return 1;
    }
    std::cout << "Connected to server" << std::endl;

    // 构造请求消息
    RpcHeader header;
    header.magic = 0x1234;
    header.version = 1;
    header.msg_type = 1;   // 请求类型
    header.msg_id = 10086;
    std::string body = "Hello RPC";
    header.body_len = body.size();

    size_t len;
    char* data = serialize_message(header, body, len);

    std::cout << "Sending " << len << " bytes..." << std::endl;
    ssize_t sent = write(sock, data, len);
    if (sent == -1) {
        perror("write");
    } else {
        std::cout << "Sent " << sent << " bytes" << std::endl;
    }

    // 接收响应
    char recv_buf[1024];
    ssize_t n = read(sock, recv_buf, sizeof(recv_buf));
    if (n > 0) {
        std::cout << "Received " << n << " bytes from server" << std::endl;
        // 可以尝试解析响应，但暂时只打印
        RpcHeader resp_header;
        std::string resp_body;
        if (deserialize_message(recv_buf, n, resp_header, resp_body)) {
            std::cout << "Response magic: 0x" << std::hex << resp_header.magic << std::dec << std::endl;
            std::cout << "Response msg_id: " << resp_header.msg_id << std::endl;
            std::cout << "Response body: " << resp_body << std::endl;
        }
    } else if (n == 0) {
        std::cout << "Server closed connection" << std::endl;
    } else {
        perror("read");
    }

    delete[] data;
    close(sock);
    return 0;
}