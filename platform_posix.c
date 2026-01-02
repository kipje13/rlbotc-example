#include "platform.h"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>


void platform_cwd_get(char *buffer, size_t size) {
    getcwd(buffer, size);
}


platform_socket_handle_t platform_socket_tcp_connect(const char *ip, uint16_t port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return -1;
    }

    /* Enable TCP_NODELAY (disables Nagle's algorithm) for lower latency */
    int tcp_nodelay = 1;
    setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&tcp_nodelay, sizeof(int));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = inet_addr(ip),
    };

    int err = connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (err == -1) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}


void platform_socket_tcp_close(platform_socket_handle_t socket) {
    close(socket);
}


int platform_socket_send(platform_socket_handle_t socket, const void *buffer, size_t size) {
    return send(socket, buffer, size, 0);
}


int platform_socket_recv(platform_socket_handle_t socket, void *buffer, size_t size) {
    return recv(socket, buffer, size, 0);
}
