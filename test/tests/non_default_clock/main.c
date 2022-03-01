#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "barectf_platform_pktring.h"
#include "barectf.h"
#include "util.h"

static uint8_t g_pktring_mem[TRACE_PKTRING_BUFFER_SIZE] = { 0 };
static barectf_stream_ctx* g_tracer = NULL;
static uint32_t g_ticks = 0;

uint32_t barectf_platform_pktring_get_clock(void)
{
    const uint32_t ticks = g_ticks;
    g_ticks += 1;
    return ticks;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    barectf_platform_pktring_init(&g_pktring_mem[0]);
    g_tracer = barectf_platform_pktring_ctx();
    assert(g_tracer != NULL);
    barectf_trace_startup(g_tracer);
    barectf_trace_shutdown(g_tracer);
    barectf_platform_pktring_fini();

    return EXIT_SUCCESS;
}
