cmake_minimum_required(VERSION 3.11)
project(rlbotc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(ExternalProject)
include(FetchContent)


# Build flatcc
FetchContent_Declare(
  flatcc_git
  GIT_REPOSITORY https://github.com/dvidelabs/flatcc.git
  GIT_TAG e3e44836c5f625b5532586ddce895f8b5e36a212
)
set(FLATCC_TEST OFF)
FetchContent_MakeAvailable(
  flatcc_git
)


# Generate RLBot flatbuffers headers
ExternalProject_Add(
  rlbot_flatbuffers_ext
  GIT_REPOSITORY https://github.com/rlbot/flatbuffers-schema.git
  GIT_TAG main
  CONFIGURE_COMMAND ""
  BUILD_COMMAND $<TARGET_FILE:flatcc_cli> -g -a <SOURCE_DIR>/schema/corepacket.fbs <SOURCE_DIR>/schema/interfacepacket.fbs
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)
ExternalProject_Get_property(rlbot_flatbuffers_ext BINARY_DIR)
ExternalProject_Get_property(rlbot_flatbuffers_ext SOURCE_DIR)


# Create a library for the RLBot flatbuffers interface, for bot applications to depend on
add_library(rlbot_flatbuffers INTERFACE)
target_include_directories(rlbot_flatbuffers
  INTERFACE ${BINARY_DIR}
  INTERFACE ${SOURCE_DIR}/include
)
target_link_libraries(rlbot_flatbuffers INTERFACE flatccrt)
add_dependencies(rlbot_flatbuffers rlbot_flatbuffers_ext)



if(WIN32)
    set(platform platform_windows.c)
    add_compile_definitions(TARGET_PLATFORM_WINDOWS)
else()
    set(platform platform_posix.c)
    add_compile_definitions(TARGET_PLATFORM_POSIX)
endif()

if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    add_compile_definitions(TARGET_ENDIAN_BIG)
elseif(CMAKE_C_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    add_compile_definitions(TARGET_ENDIAN_LITTLE)
endif()

add_executable(rlbot main.c common.c ${platform})
target_link_libraries(rlbot rlbot_flatbuffers m)

add_executable(match_runner match_runner.c common.c ${platform})
target_link_libraries(match_runner rlbot_flatbuffers)
