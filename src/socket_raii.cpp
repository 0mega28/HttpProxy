#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>

#include <memory>
#include <string>

#include "socket_raii.h"

// Socket Start
Socket::Socket() : fd_(-1) {}

Socket::Socket(int fd) : fd_(fd) {}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close_if_valid();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Socket::~Socket() noexcept {
    close_if_valid();
}

int Socket::fd() const {
    return fd_;
}

bool Socket::is_valid() const {
    return fd_ >= 0;
}

void Socket::reset(int newfd) {
    close_if_valid();
    fd_ = newfd;
}

void Socket::close_if_valid() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

// Socket End


// ServerSocket Start
ServerSocket::ServerSocket(int port): _port(port) {
    int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::string error_msg = "Socket Creation Failed: ";
        error_msg.append(strerror(errno));
        throw std::runtime_error(error_msg);
    }

    _socket.reset(server_fd);

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family = PF_INET;
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    server_sockaddr.sin_port = htons(_port);

    if (bind(_socket.fd(), (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) != 0)
    {
        std::string error_msg = "Bind Failed: ";
        error_msg.append(strerror(errno));
        throw std::runtime_error(error_msg);
    }

    if (listen(_socket.fd(), 20) != 0)
    {
        std::string error_msg = "Listen Failed: ";
        error_msg.append(strerror(errno));
        throw std::runtime_error(error_msg);
    }
}

Socket ServerSocket::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(_socket.fd(), (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd == -1)
    {
        std::string error_msg = "Accept Failed: ";
        error_msg.append(strerror(errno));
        throw std::runtime_error(error_msg);
    }

    return Socket(client_fd);
}

// ServerSocket End
