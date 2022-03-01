#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "barectf_platform_pktring.h"
#include "util.h"

#ifndef TEST_NAME
#error "TEST_NAME not defined"
#endif

#if !defined(TRACE_FILE_PATH)
#error "TRACE_FILE_PATH not defined"
#endif

static const char TRACE_FILE[] = TRACE_FILE_PATH "/" TEST_NAME ".bin";

static FILE* g_trace_file = NULL;
static size_t g_trace_pkts_written = 0;

static void append_packet_to_file(const uint8_t* packet)
{
    assert(g_trace_file != NULL);
    const size_t ret = fwrite(&packet[0], 1, TRACE_CFG_PACKET_SIZE, g_trace_file);
    assert(ret == TRACE_CFG_PACKET_SIZE);
    const int status = fflush(g_trace_file);
    assert(status == 0);
    g_trace_pkts_written += 1;
}

uint64_t util_timestamp_ns(void)
{
    struct timespec t = {0};
    const int err = clock_gettime(CLOCK_MONOTONIC, &t);
    assert(err == 0);
    const uint64_t ns = (uint64_t) t.tv_sec * 1000000000UL + (uint64_t) t.tv_nsec;
    return ns;
}

size_t util_drain_pktring_to_file(void)
{
#if !defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 0)
    (void) append_packet_to_file; /* Suppress unused function warning */
#endif

    assert(g_trace_file == NULL);
    g_trace_pkts_written = 0;

    g_trace_file = fopen(TRACE_FILE, "wb");
    assert(g_trace_file != NULL);

    while(barectf_platform_pktring_next_packet(append_packet_to_file) != 0);

    const int status = fclose(g_trace_file);
    assert(status == 0);
    g_trace_file = NULL;

    return g_trace_pkts_written;
}
