#include <iostream>
#include <string>
#include "rpc_protocol.h"   // 包含我们刚刚写的头文件

int main() {
    // 1. 构造一个请求头
    RpcHeader header;
    header.magic    = 0x1234;
    header.version  = 0x01;
    header.msg_type = 0x01;       // 请求
    header.msg_id   = 10086;
    header.body_len = 0;           // 先测试空 body

    std::string body = "";         // 空字符串

    // 2. 序列化
    size_t len;
    char* buffer = serialize_message(header, body, len);
    std::cout << "序列化后总长度: " << len << " 字节" << std::endl;

    // 3. 反序列化
    RpcHeader parsed_header;
    std::string parsed_body;
    bool ok = deserialize_message(buffer, len, parsed_header, parsed_body);

    if (ok) {
        std::cout << "反序列化成功！" << std::endl;
        std::cout << "magic: " << std::hex << parsed_header.magic << std::dec << std::endl;
        std::cout << "version: " << (int)parsed_header.version << std::endl;
        std::cout << "msg_type: " << (int)parsed_header.msg_type << std::endl;
        std::cout << "msg_id: " << parsed_header.msg_id << std::endl;
        std::cout << "body_len: " << parsed_header.body_len << std::endl;
        std::cout << "body: \"" << parsed_body << "\"" << std::endl;
    } else {
        std::cout << "反序列化失败！" << std::endl;
    }

    // 4. 释放内存
    delete[] buffer;

    return 0;
}

/**
*#include <iostream>
#include "tcp_server.h"

int main() {
    try {
        TcpServer server(8888);  // 监听 8888 端口

        // 设置连接回调（可选）
        server.setOnConnect([](int client_fd) {
            std::cout << "[Callback] Client connected, fd=" << client_fd << std::endl;
        });

        // 设置消息回调
        server.setOnMessage([](int client_fd, const char* data, size_t len) {
            std::cout << "[Callback] Message from fd " << client_fd
                      << ", length=" << len << std::endl;
            // 可以简单打印前几个字节（注意可能不是字符串）
            // 这里只是示例
        });

        // 设置关闭回调
        server.setOnClose([](int client_fd) {
            std::cout << "[Callback] Client closed, fd=" << client_fd << std::endl;
        });

        std::cout << "Starting server..." << std::endl;
        server.start();  // 阻塞运行
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
 */