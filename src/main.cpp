#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <optional>

#include "config.h"
#include "util.h"
#include "socket_raii.h"
#include "remote_socket.h"
#include "url.h"

void print_prog_info(char *prog_name)
{
    std::cout << prog_name << " v" << HttpProxy_VERSION_MAJOR << '.' << HttpProxy_VERSION_MINOR << std::endl;
}

int remote_request(void)
{
    const std::string path = "/ip";
    const std::string hostname = "httpbin.org";
    const std::string port = "80";
    std::expected<Socket, remote_socket_error> socket_or_error = get_remote_socket(hostname, port);
    if (!socket_or_error.has_value())
    {
        std::cerr << socket_or_error.error().error << std::endl;
        return 69;
    }

    Socket &client_socket = socket_or_error.value();
    std::string request_payload;
    request_payload.append("GET ").append(path).append(" HTTP/1.1\r\n").append("Host: ").append(hostname).append("\r\n").append("Accept: */*\r\n").append("Connection: close\r\n").append("\r\n");
    std::cout << "Request: \n"
              << request_payload << std::endl;

    ssize_t sent_bytes = send(client_socket.fd(), request_payload.c_str(), request_payload.size(), 0);
    if (sent_bytes == -1)
    {
        perror("send");
        return 69;
    }
    if ((size_t)sent_bytes != request_payload.size())
    {
        std::cerr << "Unable to send full payload" << std::endl;
        return 69;
    }

    // TODO ensure full reading
    std::vector<char> buffer;
    buffer.resize(1024);
    ssize_t sz;

    while ((sz = read(client_socket.fd(), buffer.data(), buffer.size())) > 0)
    {
        std::cout << std::string(buffer.data(), (size_t)sz);
    }
    std::cout << std::endl;

    return 0;
}

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
int main(int argc, char **argv)
{
    if (argc >= 1)
        print_prog_info(argv[0]);

    int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket creation");
        return 69;
    }

    Socket server_socket(server_fd);
    int port = 8080;

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family = PF_INET;
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    server_sockaddr.sin_port = htons(port);

    if (bind(server_socket.fd(), (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) != 0)
    {
        perror("bind");
        return 69;
    }

    if (listen(server_socket.fd(), 20) != 0)
    {
        perror("listen");
        return 69;
    }

    std::cout << "Server Started at port: " << port << std::endl;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_socket.fd(), (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd == -1)
    {
        perror("accept");
        return 69;
    }

    Socket client_socket(client_fd);
    std::string receive_buff;
    // TODO url size can be 2048
    receive_buff.resize(1024);
    ssize_t sz;

    // try read
    if ((sz = recv(client_socket.fd(), receive_buff.data(), receive_buff.size(), 0)) > 0)
    {
        receive_buff.resize((size_t)sz);
    }

    if (sz == -1)
    {
        std::cerr << "Error reading payload from client" << std::endl;
        return 69;
    }

    if (sz == 0)
    {
        // client closed their side of connection
    }

    std::cout << "Reading request from client" << std::endl;
    std::cout << receive_buff;
    std::cout << "\n";

    size_t request_line_end = receive_buff.find('\n');
    if (request_line_end == std::string::npos)
    {
        std::cerr << "Improper request" << std::endl;
        return 69;
    }

    // there is no validation below in parsing the request
    std::string request_line = receive_buff.substr(0, request_line_end);

    size_t verb_end = request_line.find(' ');
    if (verb_end == std::string::npos)
    {
        std::cerr << "Improper request" << std::endl;
        return 69;
    }

    std::string verb = request_line.substr(0, verb_end);

    size_t url_start = verb_end + 1;
    size_t url_end = request_line.find(' ', url_start);
    if (url_end == std::string::npos)
    {
        std::cerr << "Improper request" << std::endl;
        return 69;
    }

    std::string url = request_line.substr(url_start, url_end - url_start);

    size_t http_version_start = url_end + 1;
    size_t http_version_end = request_line.find('\r', url_end);
    // no validation
    std::string http_version = request_line.substr(http_version_start, http_version_end - http_version_start);

    std::cout << std::endl;
    std::cout << "VERB: (" << verb << "), "
              << "URL: (" << url << "), "
              << "HTTP_VERSION: (" << http_version << ")\n";

    std::optional<const proxy_url> parsed_url_op = parse_proxy_url(url);

    if (!parsed_url_op.has_value())
    {
        std::cerr << "Unable to parse url" << std::endl;
        return 69;
    }

    const proxy_url &parsed_url = parsed_url_op.value();
    std::cout << "SCHEME: (" << parsed_url.scheme << "), "
              << "HOST: (" << parsed_url.host << "), "
              << "REST: (" << parsed_url.rest << ")\n";

    // skip the first line in receive_buff
    receive_buff = receive_buff.substr(request_line_end + 1);




    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    std::cout << "Sending response to client of size: " << response.size() << std::endl;
    sz = send(client_socket.fd(), response.c_str(), response.size(), 0);
    if (sz == -1)
    {
        perror("send");
        return 69;
    }
    if ((size_t)sz != response.size())
    {
        std::cerr << "Unable to send full response" << std::endl;
        return 69;
    }

    std::cout << "Bytes written to client: " << sz << std::endl;

    return 0;
}
