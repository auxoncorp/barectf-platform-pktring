#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "barectf_platform_pktring.h"
#include "barectf.h"

static uint8_t g_pktring_mem[TRACE_PKTRING_BUFFER_SIZE] = { 0 };
static barectf_stream_ctx* g_tracer = NULL;

uint64_t barectf_platform_pktring_get_clock(void)
{
    struct timespec t = {0};
    const int err = clock_gettime(CLOCK_MONOTONIC, &t);
    assert(err == 0);
    const uint64_t ns = (uint64_t) t.tv_sec * 1000000000UL + (uint64_t) t.tv_nsec;
    return ns;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    barectf_platform_pktring_init(&g_pktring_mem[0]);

    g_tracer = barectf_platform_pktring_ctx();

    TRACE(barectf_trace_startup(g_tracer));

    TRACE(barectf_trace_shutdown(g_tracer));

    barectf_platform_pktring_fini();

    return EXIT_SUCCESS;
}
