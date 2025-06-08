#include <iostream>
#include <cstring>
#include <array>
#include <stdexcept>

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

    const char*  hostname = "httpbin.org";
    Socket client_socket;
    sockaddr_in server_address = {};
    memset(&server_address, 0, sizeof(server_address));

    std::array<char, 1024> buffer{};
    int sz;

    // move to getaddrinfo
    struct hostent* he = gethostbyname(hostname);
    if (he == nullptr)
    {
        herror("Failed to resolve host");
        return 69;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    server_address.sin_len = 0; // sin_len is ignored on most platforms, safe to leave as 0
    std::memcpy(&server_address.sin_addr, he->h_addr_list[0], (size_t) he->h_length);
    std::memset(&server_address.sin_zero, 0, sizeof(server_address.sin_zero));

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("Failed to create client socket");
        return 69;
    }
    client_socket = Socket(sock_fd);

    if (connect(client_socket.fd(), (sockaddr*) &server_address, sizeof(server_address)) != 0)
    {
        perror("Failed to connect to socket");
        return 69;
    }

    // TODO handle may not fit in buffer
    sz = snprintf(buffer.data(), buffer.size(), "GET /ip HTTP/1.1\r\n"
                                                "Host: %s\r\n"
                                                "Accept: */*\r\n"
                                                "\r\n", hostname);
    std::cout << "Request: \n" << buffer.data() << std::endl;

    send(client_socket.fd(), buffer.data(), (size_t) sz, 0);

    // TODO handle may not fit in buffer
    sz = (int) read(client_socket.fd(), buffer.data(), buffer.size());
    std::cout << std::string(buffer.data(), (size_t) sz) << std::endl;

    return 0;
}
