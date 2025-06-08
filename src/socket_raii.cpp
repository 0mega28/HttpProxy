#include "socket_raii.h"

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

Socket::~Socket() {
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
