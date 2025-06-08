#ifndef SOCKET_RAII_H
#define SOCKET_RAII_H

#include <sys/types.h>
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
    ~Socket();

    int fd() const;
    bool is_valid() const;
    void reset(int newfd = -1);

private:
    void close_if_valid();
};

#endif // SOCKET_RAII_H
