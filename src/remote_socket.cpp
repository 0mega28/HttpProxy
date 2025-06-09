#include "remote_socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <expected>
#include <memory>

std::expected<Socket, remote_socket_error> get_remote_socket(const std::string& hostname, const std::string& port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // Force IPv4 for parity with previous code.
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    int gai_ret = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (gai_ret != 0) {
        return std::unexpected(remote_socket_error{
            .type = remote_socket_error_types::addr_resolution_fail,
            .error = "Failed to resolve host: " + std::string(gai_strerror(gai_ret))
        });
    }

    // Unique pointer with custom deleter for addrinfo
    std::unique_ptr<struct addrinfo, void(*)(struct addrinfo*)> addrinfo_holder(result, freeaddrinfo);

    int sock_fd = -1;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd == -1)
            continue;

        if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(sock_fd);
        sock_fd = -1;
    }

    if (sock_fd == -1) {
        return std::unexpected(remote_socket_error{
            .type = remote_socket_error_types::connect_fail,
            .error = "Failed to create and connect client socket"
        });
    }

    return Socket(sock_fd);
}