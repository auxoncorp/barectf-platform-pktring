cmake_minimum_required(VERSION 3.5)

project(example LANGUAGES C)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 99)
endif()

file(GLOB BARECTF_CONFIG_FILES ${CMAKE_SOURCE_DIR}/config/*.yaml)

set(BARECTF_GENERATED_FILES
    ${CMAKE_SOURCE_DIR}/generated/barectf-bitfield.h
    ${CMAKE_SOURCE_DIR}/generated/barectf.c
    ${CMAKE_SOURCE_DIR}/generated/barectf.h
    ${CMAKE_SOURCE_DIR}/generated/metadata)

add_custom_command(
    OUTPUT ${BARECTF_GENERATED_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/generated
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/generated/trace
    COMMAND barectf generate
        -m ${CMAKE_SOURCE_DIR}/generated/trace
        -c ${CMAKE_SOURCE_DIR}/generated
        -H ${CMAKE_SOURCE_DIR}/generated
        -I ${CMAKE_SOURCE_DIR}/config
        ${CMAKE_SOURCE_DIR}/config/config.yaml
    DEPENDS ${BARECTF_CONFIG_FILES}
    VERBATIM)

add_custom_target(
    barectf_generated_files
    DEPENDS
    ${BARECTF_GENERATED_FILES})

# Build the generated barectf resources into a static lib
# with its own compile options
add_library(
    generated_barectf
    ${CMAKE_SOURCE_DIR}/generated/barectf.c)
target_include_directories(
    generated_barectf
    PUBLIC
    ${CMAKE_SOURCE_DIR}/generated)
target_compile_options(
    generated_barectf
    PRIVATE
    -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wmissing-include-dirs -Wstrict-prototypes
    -Wno-sign-conversion -Wno-unused-function)
add_dependencies(
    generated_barectf
    barectf_generated_files)

set(PLATFORM_SOURCES ${CMAKE_SOURCE_DIR}/../source/barectf_platform_pktring.c)
set(PLATFORM_INCLUDE ${CMAKE_SOURCE_DIR}/../source/include)

add_executable(
    example
    main.c
    ${PLATFORM_SOURCES})
target_include_directories(
    example
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PLATFORM_INCLUDE})
target_compile_options(
    example
    PRIVATE
    -Wall -Wextra -Werror -Wpedantic
    -Wsign-conversion
    -Wshadow -Wmissing-include-dirs -Wstrict-prototypes)
target_link_libraries(
    example
    PRIVATE
    generated_barectf
    rt)
target_compile_definitions(
    example
    PRIVATE
    MY_NODE_ID=0xE3)
