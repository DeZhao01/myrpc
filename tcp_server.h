#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <functional>
#include <memory>
#include <string>

// 前向声明，隐藏 epoll 相关细节
class TcpServerImpl;

class TcpServer {
public:
    // 构造函数：绑定端口，设置连接回调、消息回调
    TcpServer(int port);
    ~TcpServer();

    // 启动服务器（进入事件循环）
    void start();

    // 设置回调函数（当有新连接时调用）
    void setOnConnect(std::function<void(int client_fd)> cb);

    // 设置回调函数（当收到完整消息时调用，参数：客户端fd, 消息体数据, 长度）
    void setOnMessage(std::function<void(int client_fd, const char* data, size_t len)> cb);

    // 设置回调函数（当客户端断开时调用）
    void setOnClose(std::function<void(int client_fd)> cb);

private:
    std::unique_ptr<TcpServerImpl> impl_;
};

#endif // TCP_SERVER_H