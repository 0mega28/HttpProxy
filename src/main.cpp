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

int main(int argc, char **argv)
{
    if (argc >= 1)
        print_prog_info(argv[0]);

    int port = 8080;
    ServerSocket server_socket(port);
    std::cout << "Server Started at port: " << server_socket.port() << std::endl;

    Socket client_socket = server_socket.accept_connection();

    std::string receive_buff;
    // TODO url size can be 2048
    receive_buff.resize(1024);
    ssize_t sz;

    // try read, full read not guranteed
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

    std::expected<Socket, remote_socket_error> remote_socket_or_error = get_remote_socket(parsed_url.host, parsed_url.port.value_or("80"));
    if (!remote_socket_or_error.has_value())
    {
        std::cerr << remote_socket_or_error.error().error << std::endl;
        return 69;
    }

    Socket &remote_socket = remote_socket_or_error.value(); 
    {
        std::string request_start = verb + " " + parsed_url.rest + " " + http_version + "\r\n";
        // TODO add X-Forwarded-For header
        std::cout << "Sending to remote: [" << request_start << "]\n";
        send(remote_socket.fd(), request_start.c_str(), request_start.size(), 0);
    }
    // TODO loop
    receive_buff.resize(1024);
    if(true) {
        ssize_t sz;
        std::cout << "Sending to remote: [" << receive_buff << "]\n";
        // TODO strip headers like Proxy-Connection
        sz = send(remote_socket.fd(), receive_buff.c_str(), receive_buff.size(), 0);
        if (sz == -1)
        {
            perror("send");
            return 69;
        }
        if ((size_t)sz != receive_buff.size())
        {
            std::cerr << "Unable to send full response" << std::endl;
            return 69;
        }

        if ((sz = recv(remote_socket.fd(), receive_buff.data(), receive_buff.size(), 0)) > 0)
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
        std::cout << "Received from remote: [" << receive_buff << "]\n";

        sz = send(client_socket.fd(), receive_buff.c_str(), receive_buff.size(), 0);
        if (sz == -1)
        {
            perror("send");
            return 69;
        }
        if ((size_t)sz != receive_buff.size())
        {
            std::cerr << "Unable to send full response" << std::endl;
            return 69;
        }
    }

    return 0;
}
