#include <stdio.h>

#include <interfacepacket_builder.h>
#include <corepacket_reader.h>

#include "platform.h"
#include "common.h"

#define NUM_PLAYERS 4


static rlbot_flat_PlayerLoadout_ref_t create_bot_loadout(flatcc_builder_t *B);
static rlbot_flat_MutatorSettings_ref_t create_mutator_settings(flatcc_builder_t *B);

int main() {
    char cwd[1024] = "";
    platform_cwd_get(cwd, sizeof(cwd));

    flatcc_builder_t builder;
    flatcc_builder_init(&builder);

    platform_socket_handle_t socket = platform_socket_tcp_connect(DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT);
    if (socket == -1) {
        perror("Failed to connect to RLBot server");
        return 1;
    }

    if (rlbot_send_interface_packet(&builder, socket,
        rlbot_flat_InterfaceMessage_as_ConnectionSettings(rlbot_flat_ConnectionSettings_create(
            &builder,
            flatbuffers_string_create_str(&builder, ""),
            flatbuffers_false, // wants_ball_prediction
            flatbuffers_false, // wants_comms
            flatbuffers_false // close_between_matches
        ))
    ) == -1) {
        perror("Failed to send ConnectionSettings");
        platform_socket_tcp_close(socket);
        return 1;
    }

    rlbot_flat_CustomBot_start(&builder);
    rlbot_flat_CustomBot_name_add(&builder, flatbuffers_string_create_str(&builder, "c_example_bot"));
    rlbot_flat_CustomBot_agent_id_add(&builder, flatbuffers_string_create_str(&builder, "c_example_bot"));
    rlbot_flat_CustomBot_root_dir_add(&builder, flatbuffers_string_create_str(&builder, cwd));
    rlbot_flat_CustomBot_run_command_add(&builder, flatbuffers_string_create_str(&builder, "./rlbot"));
    rlbot_flat_CustomBot_hivemind_add(&builder, flatbuffers_true);
    rlbot_flat_CustomBot_ref_t bot_config = rlbot_flat_CustomBot_end(&builder);

    rlbot_flat_PlayerConfiguration_ref_t player_configs[NUM_PLAYERS];
    for (int i = 0; i < NUM_PLAYERS; i++) {
        player_configs[i] = rlbot_flat_PlayerConfiguration_create(
            &builder,
            rlbot_flat_PlayerClass_as_CustomBot(bot_config),
            i & 1, /* team */
            i /* id */
        );
    }

    rlbot_flat_PlayerConfiguration_vec_ref_t player_config_vec =
        rlbot_flat_PlayerConfiguration_vec_create(&builder, player_configs, NUM_PLAYERS);

    rlbot_flat_ScriptConfiguration_vec_ref_t script_config_vec =
        rlbot_flat_ScriptConfiguration_vec_create(&builder, NULL, 0);


    rlbot_flat_MatchConfiguration_ref_t match_config = rlbot_flat_MatchConfiguration_create(
        &builder,
        rlbot_flat_Launcher_Steam,
        flatbuffers_string_create_str(&builder, ""), // launcher_arg
        flatbuffers_true, // auto_start_agents
        flatbuffers_true, // wait_for_agents
        flatbuffers_string_create_str(&builder, "Stadium_P"), // game_map_upk
        player_config_vec,
        script_config_vec,
        rlbot_flat_GameMode_Soccar,
        flatbuffers_false, // skip_replays
        flatbuffers_false, // instant_start
        create_mutator_settings(&builder),
        rlbot_flat_ExistingMatchBehavior_Restart,
        rlbot_flat_DebugRendering_OnByDefault,
        flatbuffers_true, // enable_state_setting
        flatbuffers_false, // auto_save_replay
        flatbuffers_false // freeplay
    );

    int err = rlbot_send_interface_packet(&builder, socket, rlbot_flat_InterfaceMessage_as_MatchConfiguration(match_config));

    platform_socket_tcp_close(socket);

    if (err == -1) {
        perror("Failed to send MatchConfiguration");
        return 1;
    } else {
        return 0;
    }
}


static rlbot_flat_MutatorSettings_ref_t create_mutator_settings(flatcc_builder_t *B) {
    rlbot_flat_MutatorSettings_start(B);
    rlbot_flat_MutatorSettings_match_length_add(B, rlbot_flat_MatchLengthMutator_Unlimited);
    return rlbot_flat_MutatorSettings_end(B);
}
