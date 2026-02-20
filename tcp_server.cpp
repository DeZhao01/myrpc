//
// Created by Lenovo on 2026/2/19.
//
#include "tcp_server.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <unordered_map>

// 每个客户端连接对应的接收缓冲区（用于处理粘包）
struct ConnBuffer {
    std::vector<char> data;   // 已接收但尚未解析的数据
    ConnBuffer() { data.reserve(4096); }
};

class TcpServerImpl {
public:
    int listen_fd_;
    int epoll_fd_;
    int port_;
    bool running_;
    std::unordered_map<int, ConnBuffer> buffers_;   // fd -> 缓冲区
    std::vector<struct epoll_event> events_;        // 用于 epoll_wait

    // 回调函数
    std::function<void(int)> on_connect_;
    std::function<void(int, const char*, size_t)> on_message_;
    std::function<void(int)> on_close_;

    TcpServerImpl(int port)
        : port_(port), running_(false), listen_fd_(-1), epoll_fd_(-1) {
        events_.resize(64);  // 初始事件数组大小
    }

    ~TcpServerImpl() {
        if (listen_fd_ != -1) close(listen_fd_);
        if (epoll_fd_ != -1) close(epoll_fd_);
    }

    bool init() {
        // 1. 创建监听 socket
        listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd_ == -1) {
            perror("socket");
            return false;
        }

        // 2. 设置端口复用（方便调试）
        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 3. 绑定地址
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            perror("bind");
            return false;
        }

        // 4. 开始监听
        if (listen(listen_fd_, 128) == -1) {
            perror("listen");
            return false;
        }

        // 5. 创建 epoll 实例
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            perror("epoll_create1");
            return false;
        }

        // 6. 将监听 fd 加入 epoll（边缘触发，方便后续练习优化）
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发
        ev.data.fd = listen_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev) == -1) {
            perror("epoll_ctl: listen_fd");
            return false;
        }

        std::cout << "Server listening on port " << port_ << std::endl;
        return true;
    }

    void start() {
        running_ = true;
        while (running_) {
            int nfds = epoll_wait(epoll_fd_, events_.data(), events_.size(), -1);
            if (nfds == -1) {
                perror("epoll_wait");
                break;
            }
            for (int i = 0; i < nfds; ++i) {
                int fd = events_[i].data.fd;
                if (fd == listen_fd_) {
                    // 有新连接
                    handleAccept();
                } else {
                    // 普通客户端 fd，有数据可读
                    if (events_[i].events & EPOLLIN) {
                        handleRead(fd);
                    }
                    // 如果有 EPOLLOUT 等事件，这里暂时忽略，先实现基本功能
                }
            }
        }
    }

    void handleAccept() {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        // 由于边缘触发，需要循环 accept 直到返回 EAGAIN
        while (true) {
            int client_fd = accept4(listen_fd_, (struct sockaddr*)&client_addr, &addrlen, SOCK_NONBLOCK);
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;  // 没有更多新连接
                } else {
                    perror("accept");
                    break;
                }
            }

            // 将新客户端 fd 加入 epoll（边缘触发）
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = client_fd;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                perror("epoll_ctl: client_fd");
                close(client_fd);
                continue;
            }

            // 为这个连接创建缓冲区
            buffers_.emplace(client_fd, ConnBuffer());

            // 调用用户回调（如果有）
            if (on_connect_) on_connect_(client_fd);

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
            std::cout << "New connection from " << ip << ":" << ntohs(client_addr.sin_port)
                      << ", fd=" << client_fd << std::endl;
        }
    }

    void handleRead(int fd) {
        // 获取该连接的缓冲区
        auto it = buffers_.find(fd);
        if (it == buffers_.end()) {
            // 没有缓冲区说明该连接已异常，直接关闭
            close(fd);
            return;
        }
        ConnBuffer& buf = it->second;

        char tmp[4096];
        bool closed = false;
        while (true) {
            ssize_t n = read(fd, tmp, sizeof(tmp));
            if (n > 0) {
                // 将数据追加到缓冲区
                buf.data.insert(buf.data.end(), tmp, tmp + n);
            } else if (n == 0) {
                // 连接关闭
                closed = true;
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 当前没有更多数据可读，退出循环
                    break;
                } else {
                    // 读出错，关闭连接
                    perror("read");
                    closed = true;
                    break;
                }
            }
        }

        // 检查是否有完整消息（按我们的协议：固定头部12字节，然后根据body_len读取）
        // 这里我们暂时不解析协议，只把数据原样交给上层回调（为了通用性）
        // 但为了演示，我们简单打印收到的数据长度
        if (!buf.data.empty()) {
            std::cout << "Received " << buf.data.size() << " bytes from fd " << fd << std::endl;
            // 调用消息回调，传入原始数据（后续我们会在这里解析协议）
            if (on_message_) {
                on_message_(fd, buf.data.data(), buf.data.size());
            }
            // 注意：这里我们不清空缓冲区，只是简单回调。实际应该按协议切分消息。
            // 但为了第二步的简洁，我们先只通知上层有数据到达，具体协议解析在第三步做。
            // 所以这里暂时保留缓冲区，不清空。
        }

        if (closed) {
            // 连接关闭，清理资源
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            buffers_.erase(fd);
            if (on_close_) on_close_(fd);
            std::cout << "Connection closed, fd=" << fd << std::endl;
        }
    }
};

// ---------- TcpServer 包装类实现 ----------
TcpServer::TcpServer(int port) : impl_(std::make_unique<TcpServerImpl>(port)) {
    if (!impl_->init()) {
        throw std::runtime_error("TcpServer init failed");
    }
}

TcpServer::~TcpServer() = default;

void TcpServer::start() {
    impl_->start();
}

void TcpServer::setOnConnect(std::function<void(int)> cb) {
    impl_->on_connect_ = std::move(cb);
}

void TcpServer::setOnMessage(std::function<void(int, const char*, size_t)> cb) {
    impl_->on_message_ = std::move(cb);
}

void TcpServer::setOnClose(std::function<void(int)> cb) {
    impl_->on_close_ = std::move(cb);
}