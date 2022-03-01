cmake_minimum_required(VERSION 3.5)

macro(add_c_test NAME COND)
    if(${COND})
        set(BARECTF_GENERATED_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/barectf-bitfield.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/barectf.c
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/barectf.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/trace/metadata)
        add_custom_command(
            OUTPUT ${BARECTF_GENERATED_FILES}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/trace
            COMMAND barectf generate
                -m ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/trace
                -c ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated
                -H ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated
                ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/config.yaml
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/config.yaml
            VERBATIM)
        add_custom_target(
            barectf_generated_files_${NAME}
            DEPENDS
            ${BARECTF_GENERATED_FILES})

        add_library(
            generated_barectf_${NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/barectf.c)
        target_include_directories(
            generated_barectf_${NAME}
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated)
        target_compile_options(
            generated_barectf_${NAME}
            PRIVATE
            -Wall -Wextra -Werror -Wpedantic
            -Wshadow -Wmissing-include-dirs -Wstrict-prototypes
            -Wno-sign-conversion -Wno-unused-function)
        add_dependencies(
            generated_barectf_${NAME}
            barectf_generated_files_${NAME})

        add_executable(
            ${NAME}
            ${NAME}/main.c
            ../../source/barectf_platform_pktring.c
            common/util.c)
        target_compile_options(
            ${NAME}
            PRIVATE
            -Wall -Wextra -Werror -Wpedantic
            -Wsign-conversion
            -Wshadow -Wmissing-include-dirs -Wstrict-prototypes)
        target_include_directories(
            ${NAME}
            PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${NAME}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../source/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>)
        target_link_libraries(
            ${NAME}
            PRIVATE
            generated_barectf_${NAME}
            rt)
        target_compile_definitions(
            ${NAME}
            PRIVATE
            TEST_NAME="${NAME}"
            TRACE_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/generated/trace"
            _TRACE_INTERNAL_TEST=1)
        add_test(NAME ${NAME} COMMAND ${NAME})
    endif()
endmacro()

add_c_test(pktring ON)
add_c_test(non_default_clock ON)
add_c_test(single_pkt_context ON)
add_c_test(flush_pkt ON)
add_c_test(non_default_stream ON)
add_c_test(buffer_fill_mode ON)
add_c_test(critical_sections ON)
add_c_test(no_op ON)
