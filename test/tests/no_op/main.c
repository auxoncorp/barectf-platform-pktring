#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "barectf_platform_pktring.h"
#include "barectf.h"
#include "util.h"

static barectf_stream_ctx* g_tracer = NULL;
#if defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1)
static uint8_t g_pktring_mem[TRACE_PKTRING_BUFFER_SIZE] = { 0 };

uint64_t barectf_platform_pktring_get_clock(void)
{
    assert(0);
    return 0;
}
#endif

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    barectf_platform_pktring_init(&g_pktring_mem[0]);

    g_tracer = barectf_platform_pktring_ctx();
    assert(g_tracer == NULL); /* When tracing not enabled, the no-op variant returns NULL */

    TRACE(barectf_trace_startup(g_tracer));
    
    TRACE(barectf_trace_not_a_real_thing(g_tracer));

    TRACE(barectf_trace_shutdown(g_tracer));

    barectf_platform_pktring_flush();

    barectf_platform_pktring_fini();

    return EXIT_SUCCESS;
}
