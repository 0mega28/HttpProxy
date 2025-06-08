#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "config.h"
#include "util.h"
#include "socket_raii.h"

void print_prog_info(char *prog_name)
{
    std::cout << prog_name << " v" << HttpProxy_VERSION_MAJOR << '.' << HttpProxy_VERSION_MINOR << std::endl;
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    print_prog_info(argv[0]);

    const char* hostname = "httpbin.org";
    const char* path = "/ip";

    std::array<char, 1024> buffer{};
    int sz;

    // Use getaddrinfo with unique_ptr and custom deleter
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // Force IPv4 for parity with previous code.
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    int gai_ret = getaddrinfo(hostname, "80", &hints, &result);
    if (gai_ret != 0) {
        std::cerr << "Failed to resolve host: " << gai_strerror(gai_ret) << std::endl;
        return 69;
    }

    // Unique pointer with custom deleter for addrinfo
    std::unique_ptr<struct addrinfo, void(*)(struct addrinfo*)> addrinfo_holder(result, freeaddrinfo);

    int sock_fd = -1;
    struct addrinfo* rp = result;
    for (; rp != nullptr; rp = rp->ai_next) {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd == -1)
            continue;

        if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; // Success

        close(sock_fd);
        sock_fd = -1;
    }

    if (sock_fd == -1) {
        std::cerr << "Failed to create and connect client socket" << std::endl;
        return 69;
    }
    Socket client_socket(sock_fd);

    std::string request_payload; 
    request_payload.append("GET ").append(path).append(" HTTP/1.1\r\n")
                    .append("Host: ").append(hostname).append("\r\n")
                    .append("Accept: */*\r\n")
                    .append("\r\n");
    std::cout << "Request: \n" << request_payload << std::endl;

    // TODO handle may not fit in buffer and check return sz sent
    ssize_t sent_bytes = send(client_socket.fd(), request_payload.c_str(), request_payload.size(), 0);
    if (sent_bytes == -1) {
        perror("send");
        return 69;
    }
    if ((size_t) sent_bytes != request_payload.size()) {
        std::cerr << "Unable to send full payload" << std::endl;
        return 69;
    }

    sz = (int) read(client_socket.fd(), buffer.data(), buffer.size());
    std::cout << std::string(buffer.data(), (size_t) sz) << std::endl;

    return 0;
}
