#ifndef SOCKET_RAII_H
#define SOCKET_RAII_H

#include <sys/socket.h>
#include <unistd.h>

class Socket {
    int fd_;

public:
    Socket();
    explicit Socket(int fd);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    ~Socket() noexcept;

    int fd() const;
    bool is_valid() const;
    void reset(int newfd = -1);

private:
    void close_if_valid();
};

class ServerSocket {
    Socket _socket;
    int _port;

public:
    ServerSocket() = delete;
    explicit ServerSocket(int port);
    ServerSocket(const ServerSocket&) = delete;
    ServerSocket& operator=(const ServerSocket&) = delete;
    ServerSocket(ServerSocket&& other) noexcept = default;
    ServerSocket& operator=(ServerSocket&& other) noexcept = default;
    ~ServerSocket() noexcept = default;

    Socket accept_connection();

    int port() const noexcept { return _port; }
};

#endif // SOCKET_RAII_H
