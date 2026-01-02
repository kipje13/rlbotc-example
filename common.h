#ifndef COMMON_H
#define COMMON_H

#include <flatcc/flatcc_builder.h>
#include <interfacepacket_builder.h>

#include "platform.h"

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 23234

/* Endianness swap functions */
#ifdef TARGET_ENDIAN_LITTLE
#define host_to_be16(n) (((n) >> 8) | ((n & 0xFF) << 8))
#elif defined (TARGET_ENDIAN_BIG)
#define host_to_be16(n) (n)
#else
#error "No valid target platform defined"
#endif
#define be16_to_host(n) host_to_be16(n)

extern int rlbot_send_interface_packet(flatcc_builder_t *B, platform_socket_handle_t socket, rlbot_flat_InterfaceMessage_union_ref_t message);

#endif
