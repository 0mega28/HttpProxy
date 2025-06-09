#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>

#include "config.h"
#include "util.h"
#include "socket_raii.h"
#include "remote_socket.h"

void print_prog_info(char *prog_name)
{
    std::cout << prog_name << " v" << HttpProxy_VERSION_MAJOR << '.' << HttpProxy_VERSION_MINOR << std::endl;
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    print_prog_info(argv[0]);

    const std::string path = "/ip";
    const std::string hostname = "httpbin.org";
    const std::string port = "80";
    std::expected<Socket, remote_socket_error> socket_or_error = get_remote_socket(hostname, port);
    if (!socket_or_error.has_value()) {
        std::cerr << socket_or_error.error().error << std::endl;
        return 69;
    }

    Socket& client_socket = socket_or_error.value();
    std::string request_payload; 
    request_payload.append("GET ").append(path).append(" HTTP/1.1\r\n")
                    .append("Host: ").append(hostname).append("\r\n")
                    .append("Accept: */*\r\n")
                    .append("Connection: close\r\n")
                    .append("\r\n");
    std::cout << "Request: \n" << request_payload << std::endl;

    ssize_t sent_bytes = send(client_socket.fd(), request_payload.c_str(), request_payload.size(), 0);
    if (sent_bytes == -1) {
        perror("send");
        return 69;
    }
    if ((size_t) sent_bytes != request_payload.size()) {
        std::cerr << "Unable to send full payload" << std::endl;
        return 69;
    }

    // TODO ensure full reading
    std::array<char, 1024> buffer{};
    ssize_t sz = (int) read(client_socket.fd(), buffer.data(), buffer.size());
    std::cout << std::string(buffer.data(), (size_t) sz) << std::endl;

    return 0;
}
