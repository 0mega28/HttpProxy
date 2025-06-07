#include <iostream>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


#include "config.h"

#define UNUSED(x) (void)(x)

void print_prog_info(char *prog_name)
{
    std::cout << prog_name << " v" << HttpProxy_VERSION_MAJOR << '.' << HttpProxy_VERSION_MINOR << std::endl;
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    print_prog_info(argv[0]);

    int exit_code = 0;

    // Resolve "httpbin.org" to IPv4 address using gethostbyname
    const char*  hostname = "httpbin.org";
    int client_socket = 0;
    sockaddr_in server_address = {};
    memset(&server_address, 0, sizeof(server_address));

    char buffer[1024];
    int sz;

    struct hostent* he = gethostbyname(hostname);
    if (he == nullptr)
    {
        herror("Failed to resolve host");
        exit_code = 69;
        goto end;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    server_address.sin_len = 0; // sin_len is ignored on most platforms, safe to leave as 0
    std::memcpy(&server_address.sin_addr, he->h_addr_list[0], (size_t) he->h_length);
    std::memset(&server_address.sin_zero, 0, sizeof(server_address.sin_zero));

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create client socket");
        exit_code = 69;
        goto end;
    }

    if (connect(client_socket, (sockaddr*) &server_address, sizeof(server_address)) != 0)
    {
        perror("Failed to connect to socket");
        exit_code = 69;
        goto end;
    }

    // maybe fully not fill in buffer
    sz = snprintf(buffer, sizeof(buffer), "GET /ip HTTP/1.1\r\n" 
                                        "Host: %s\r\n"
                                        "Accept: */*\r\n"
                                        "\r\n", hostname);
    std::cout << "Request: \n" << buffer << std::endl;

    send(client_socket, buffer, (size_t) sz, 0);

    sz = (int) read(client_socket, buffer, sizeof(buffer));
    std::cout << std::string(buffer, (size_t) sz) << std::endl;

end:
    close(client_socket);
    freehostent(he);
    return exit_code;
}
