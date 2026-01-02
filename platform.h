#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <stdint.h>

extern void platform_cwd_get(char *buffer, size_t size);

#ifdef TARGET_PLATFORM_POSIX
typedef int platform_socket_handle_t;
#elif defined (TARGET_PLATFORM_WINDOWS)
#error "Not implemented"
#else
#error "No valid target platform defined"
#endif

extern platform_socket_handle_t platform_socket_tcp_connect(const char *ip, uint16_t port);
extern void platform_socket_tcp_close(platform_socket_handle_t socket);

extern int platform_socket_send(platform_socket_handle_t socket, const void *buffer, size_t size);
extern int platform_socket_recv(platform_socket_handle_t socket, void *buffer, size_t size);

#endif
