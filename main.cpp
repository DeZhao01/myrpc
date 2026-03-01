#include <iostream>
#include <unistd.h>
#include "tcp_server.h"
#include "rpc_protocol.h"

int main() {
    try {
        TcpServer server(8888);

        server.setOnConnect([](int client_fd) {
            std::cout << "[Connect] fd=" << client_fd << std::endl;
        });

        server.setOnMessage([](int client_fd, const char* data, size_t len) {
            std::cout << "[Message] from fd " << client_fd << ", length=" << len << std::endl;

            RpcHeader header;
            std::string body;
            if (deserialize_message(data, len, header, body)) {
                std::cout << "  -> Parse success!" << std::endl;
                std::cout << "     magic: 0x" << std::hex << header.magic << std::dec << std::endl;
                std::cout << "     version: " << (int)header.version << std::endl;
                std::cout << "     msg_type: " << (int)header.msg_type << std::endl;
                std::cout << "     msg_id: " << header.msg_id << std::endl;
                std::cout << "     body_len: " << header.body_len << std::endl;
                if (header.body_len > 0) {
                    std::cout << "     body: \"" << body << "\"" << std::endl;
                }

                if (header.msg_type == 1) {
                    RpcHeader resp_header;
                    resp_header.magic = 0x1234;
                    resp_header.version = 1;
                    resp_header.msg_type = 2;
                    resp_header.msg_id = header.msg_id;
                    std::string resp_body = "Hello from server";
                    resp_header.body_len = resp_body.size();

                    size_t resp_len;
                    char* resp_data = serialize_message(resp_header, resp_body, resp_len);
                    ssize_t sent = write(client_fd, resp_data, resp_len);
                    if (sent == -1) perror("write");
                    else std::cout << "  -> Sent response, " << sent << " bytes" << std::endl;
                    delete[] resp_data;
                }
            } else {
                std::cout << "  -> Parse failed (incomplete or invalid)" << std::endl;
            }
        });

        server.setOnClose([](int client_fd) {
            std::cout << "[Close] fd=" << client_fd << std::endl;
        });

        std::cout << "Starting RPC server..." << std::endl;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}