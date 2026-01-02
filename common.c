#include "common.h"

int rlbot_send_interface_packet(flatcc_builder_t *B, platform_socket_handle_t socket, rlbot_flat_InterfaceMessage_union_ref_t message) {
    void        *buf;
    size_t      buf_size;
    uint16_t    buf_size_be16;
    int         err;

    rlbot_flat_InterfacePacket_create_as_root(B, message);

    buf = flatcc_builder_finalize_buffer(B, &buf_size);
    assert(buf_size <= UINT16_MAX);

    buf_size_be16 = host_to_be16(buf_size);
    err = platform_socket_send(socket, &buf_size_be16, sizeof(buf_size_be16));
    if (err != -1)
        err = platform_socket_send(socket, buf, buf_size);

    flatcc_builder_reset(B);

    return err;
}
