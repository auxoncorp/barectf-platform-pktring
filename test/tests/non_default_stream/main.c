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

uint64_t barectf_platform_pktring_get_clock(void)
{
    return util_timestamp_ns();
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    barectf_platform_pktring_init(&g_pktring_mem[0]);

    g_tracer = barectf_platform_pktring_ctx();
    assert(g_tracer != NULL);

    barectf_node3_trace_startup(g_tracer);

    barectf_node3_trace_shutdown(g_tracer);

    barectf_platform_pktring_fini();

    const size_t num_pkts = util_drain_pktring_to_file();
    assert(num_pkts == 1);

    return EXIT_SUCCESS;
}
