#ifndef REMOTE_SOCKET_H
#define REMOTE_SOCKET_H

#include "socket_raii.h"
#include <expected>
#include <string>

enum class remote_socket_error_types {
    addr_resolution_fail,
    connect_fail
};

struct remote_socket_error {
    enum remote_socket_error_types type;
    std::string error;
};


std::expected<Socket, remote_socket_error> get_remote_socket(const std::string& hostname, const std::string& port);

#endif // REMOTE_SOCKET_H