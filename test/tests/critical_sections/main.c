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
static int g_critical_section_nesting_depth = 0;
static int g_critical_section_was_used = 0;

void alloc_critical_section(void)
{
    /* Do nothing */
}

void enter_critical_section(void)
{
    g_critical_section_nesting_depth += 1;
    g_critical_section_was_used = 1;
}

void exit_critical_section(void)
{
    g_critical_section_nesting_depth -= 1;
    g_critical_section_was_used = 1;
}

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

    int i;
    for(i = 0; i < 40; i += 1)
    {
        TRACE(barectf_trace_my_event(g_tracer));
    }

    barectf_platform_pktring_fini();

    const size_t num_pkts = util_drain_pktring_to_file();
    assert(num_pkts == TRACE_CFG_NUM_PACKETS);

    assert(g_critical_section_was_used == 1);
    assert(g_critical_section_nesting_depth == 0);

    return EXIT_SUCCESS;
}
