#include <stdio.h>
#include <math.h>

#include <interfacepacket_builder.h>
#include <corepacket_reader.h>

#include "platform.h"
#include "common.h"


typedef struct bot_info {
    int32_t id;
    int32_t index;
} bot_info_t;


static struct rlbot_flat_ControllerState compute_inputs(rlbot_flat_GamePacket_table_t gamepacket, int32_t bot_id);
static size_t get_bot_info(rlbot_flat_ControllableTeamInfo_table_t controllable_info, bot_info_t *bot_array);

static int send_ready_message(flatcc_builder_t *B, platform_socket_handle_t socket);
static int send_input_message(flatcc_builder_t *B, platform_socket_handle_t socket, rlbot_flat_ControllerState_t *input, int32_t bot_id);


int main() {
    bot_info_t bot_array[64];
    size_t num_bots = 0;

    flatcc_builder_t builder;
    flatcc_builder_init(&builder);


    /* Get the IP address of the RLBot server, this environment variable is set if this bot is started by the RLBot server */
    const char *server_ip = getenv("RLBOT_SERVER_IP");
    if (server_ip == NULL)
        server_ip = DEFAULT_SERVER_IP;

    /* Similarly, the server port is also provided as an environment variable */
    const char *server_port_str = getenv("RLBOT_SERVER_PORT");
    uint16_t server_port = (server_port_str == NULL) ? DEFAULT_SERVER_PORT : atoi(server_port_str);


    /* Open a TCP connection to the server */
    platform_socket_handle_t socket = platform_socket_tcp_connect(server_ip, server_port);
    if (socket == -1) {
        perror("Failed to connect to RLBot server");
        return 1;
    }


    /* Notify that we are a bot agent for the "c_example_bot" agent id */
    if (rlbot_send_interface_packet(&builder, socket,
        rlbot_flat_InterfaceMessage_as_ConnectionSettings(rlbot_flat_ConnectionSettings_create(
            &builder,
            flatbuffers_string_create_str(&builder, "c_example_bot"),
            flatbuffers_false, /* wants_ball_prediction */
            flatbuffers_false, /* wants_comms */
            flatbuffers_true /* close_between_matches */
        ))
    ) == -1) {
        perror("Failed to send ConnectionSettings");
        goto exit_error;
    }


    while (1) {
        uint8_t *receive_buffer[4096];
        uint16_t packet_size_be16;
        uint16_t packet_size;
        int err;

        err = platform_socket_recv(socket, &packet_size_be16, sizeof(packet_size_be16));
        if (err == -1) {
            perror("Socket receive error");
            goto exit_error;
        } else if (err == 0) {
            goto exit_clean;
        }

        packet_size = be16_to_host(packet_size_be16);
        assert(packet_size <= sizeof(receive_buffer));

        err = platform_socket_recv(socket, receive_buffer, packet_size);
        if (err == -1) {
            perror("Socket receive error");
            goto exit_error;
        } else if (err == 0) {
            goto exit_clean;
        }

        rlbot_flat_CorePacket_table_t packet = rlbot_flat_CorePacket_as_root(receive_buffer);
        rlbot_flat_CoreMessage_union_type_t packet_type = (rlbot_flat_CorePacket_message_type_get(packet));


        switch (packet_type) {
            case rlbot_flat_CoreMessage_DisconnectSignal:
                goto exit_clean;

            /* The FieldInfo message sends static information about the stadium (e.g. goals, boost locations, etc...) */
            case rlbot_flat_CoreMessage_FieldInfo:
                err = send_ready_message(&builder, socket);
                if (err == -1) {
                    perror("Failed to send ready message");
                    goto exit_error;
                }
                break;

            /* The ControllableTeamInfo message provides a list of bots that we can control */
            case rlbot_flat_CoreMessage_ControllableTeamInfo:
                num_bots = get_bot_info(rlbot_flat_CorePacket_message_get(packet), bot_array);
                break;

            /* The GamePacket contains the dynamic state of the game and is send continuously by the RLBot server */
            case rlbot_flat_CoreMessage_GamePacket:
                /* Loop over the bots that we control and generate send controller inputs based on the current state of the game */
                for (int i = 0; i < num_bots; i++) {
                    struct rlbot_flat_ControllerState inputs = compute_inputs((rlbot_flat_GamePacket_table_t)rlbot_flat_CorePacket_message_get(packet), bot_array[i].index);

                    err = send_input_message(&builder, socket, &inputs, bot_array[i].index);
                    if (err == -1) {
                        perror("Failed to send input message");
                        goto exit_error;
                    }
                }
                break;

            default:
                break;
        }
    }

exit_clean:
    platform_socket_tcp_close(socket);
    return 0;

exit_error:
    platform_socket_tcp_close(socket);
    return 1;
}


static struct rlbot_flat_ControllerState compute_inputs(rlbot_flat_GamePacket_table_t gamepacket, int32_t bot_index) {
    struct rlbot_flat_ControllerState inputs = { 0 };

    /* Get the player info associated with the given bot index */
    rlbot_flat_PlayerInfo_vec_t players = rlbot_flat_GamePacket_players_get(gamepacket);
    rlbot_flat_PlayerInfo_table_t player = rlbot_flat_PlayerInfo_vec_at(players, bot_index);
    rlbot_flat_Physics_struct_t car_physics = rlbot_flat_PlayerInfo_physics_get(player);


    rlbot_flat_BallInfo_vec_t balls = rlbot_flat_GamePacket_balls_get(gamepacket);
    if (rlbot_flat_BallInfo_vec_len(balls) == 0) {
        return inputs;
    }

    rlbot_flat_BallInfo_table_t ball = rlbot_flat_BallInfo_vec_at(balls, 0);
    rlbot_flat_Physics_struct_t ball_physics = rlbot_flat_BallInfo_physics_get(ball);


    /* Simple ATBA (Always Towards Ball Agent) controller */
    /* car right vector */
    float v_r_x = cosf(car_physics->rotation.yaw + M_PI_2);
    float v_r_y = sinf(car_physics->rotation.yaw + M_PI_2);

    /* vector from car to ball */
    float v_ball_x = ball_physics->location.x - car_physics->location.x;
    float v_ball_y = ball_physics->location.y - car_physics->location.y;

    inputs.throttle = 1;
    inputs.steer = v_r_x * v_ball_x + v_r_y * v_ball_y;

    return inputs;
}


static size_t get_bot_info(rlbot_flat_ControllableTeamInfo_table_t controllable_info, bot_info_t *bot_array) {
    rlbot_flat_ControllableInfo_vec_t   bots;
    rlbot_flat_ControllableInfo_table_t bot;
    size_t i;

    if (!rlbot_flat_ControllableTeamInfo_controllables_is_present(controllable_info)) {
        return 0;
    }

    bots = rlbot_flat_ControllableTeamInfo_controllables_get(controllable_info);
    for (i = 0; i < rlbot_flat_ControllableInfo_vec_len(bots); i++) {
        bot = rlbot_flat_ControllableInfo_vec_at(bots, i);
        bot_array[i].index = rlbot_flat_ControllableInfo_index_get(bot);
        bot_array[i].id = rlbot_flat_ControllableInfo_identifier_get(bot);
    }

    return rlbot_flat_ControllableInfo_vec_len(bots);
}


int send_ready_message(flatcc_builder_t *B, platform_socket_handle_t socket) {
    return rlbot_send_interface_packet(B, socket, rlbot_flat_InterfaceMessage_as_InitComplete(rlbot_flat_InitComplete_create(B)));
}


int send_input_message(flatcc_builder_t *B, platform_socket_handle_t socket, rlbot_flat_ControllerState_t *input, int32_t bot_id) {
    return rlbot_send_interface_packet(B, socket, rlbot_flat_InterfaceMessage_as_PlayerInput(rlbot_flat_PlayerInput_create(B, bot_id, input)));
}
