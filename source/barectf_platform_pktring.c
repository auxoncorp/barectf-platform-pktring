#include <stdint.h>
#include <stdio.h>

#include "barectf_platform_pktring.h"

#if defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1)

#include "barectf.h"

#if defined(_TRACE_INTERNAL_TEST)
#define API_VIZ
#else
#define API_VIZ static
#endif

#if !defined(TRACE_MEMSET)
#include <string.h>
#define TRACE_MEMSET(s, c, n) memset(s, c, n)
#endif

/* Use the 'default' clock type if TRACE_CFG_CLOCK_TYPE is not specified */
#if !defined(TRACE_CFG_CLOCK_TYPE)
#define TRACE_CFG_CLOCK_TYPE default
#endif

#if defined(TRACE_CFG_PACKET_CONTEXT_FIELD)
    #define bctf_open_packet(ctx, pctx) TRACE_CAT3(barectf_, TRACE_CFG_STREAM_TYPE, _open_packet)(ctx, pctx)
#else
    #define bctf_open_packet(ctx) TRACE_CAT3(barectf_, TRACE_CFG_STREAM_TYPE, _open_packet)(ctx)
#endif
#define bctf_close_packet(ctx) TRACE_CAT3(barectf_, TRACE_CFG_STREAM_TYPE, _close_packet)(ctx)

/* Simple ring buffer for CTF packets */
#define PKTRING_LEN_MASK (TRACE_CFG_NUM_PACKETS - 1)
typedef struct
{
    uint16_t len;
    uint16_t write_at;
    uint16_t read_from;
} pktring_s;

static barectf_stream_ctx g_barectf_stream_ctx = { 0 };
static pktring_s g_barectf_pktring = { 0 };
static uint8_t* g_barectf_pktring_mem = NULL;

/* pktring API */
API_VIZ void pktring_init(uint8_t* pktring_buffer);
API_VIZ uint16_t pktring_length(void);
API_VIZ uint8_t* pktring_grant(void);
API_VIZ void pktring_commit(void);
API_VIZ const uint8_t* pktring_read(void);
API_VIZ void pktring_release(void);

static TRACE_CFG_CLOCK_C_TYPE get_clock(void* data)
{
    (void) data;
    return barectf_platform_pktring_get_clock();
}

static int is_backend_full(void* data)
{
    (void) data;
#if defined(TRACE_CFG_BUFFER_MODE_OVERWRITE)
    /* Never "full" in overwrite mode */
    (void) pktring_length; /* Suppress unused-function warning */
    return 0;
#elif defined(TRACE_CFG_BUFFER_MODE_FILL)
    return pktring_length() == TRACE_CFG_NUM_PACKETS;
#else
#error "Buffer mode not selected, define either TRACE_CFG_BUFFER_MODE_OVERWRITE or TRACE_CFG_BUFFER_MODE_FILL"
#endif
}

static void open_packet(void* data)
{
    (void) data;

    /* Reserve a pkt entry in the pktring */
    uint8_t* pkt = pktring_grant();
    TRACE_ASSERT(pkt != NULL);
    TRACE_MEMSET(pkt, 0, TRACE_CFG_PACKET_SIZE);

    /* Use the pkt as the CTF packet buffer */
    barectf_packet_set_buf(&g_barectf_stream_ctx, pkt, TRACE_CFG_PACKET_SIZE);

#if defined(TRACE_CFG_PACKET_CONTEXT_FIELD)
    bctf_open_packet(&g_barectf_stream_ctx, TRACE_CFG_PACKET_CONTEXT_FIELD);
#else
    bctf_open_packet(&g_barectf_stream_ctx);
#endif
}

static void close_packet(void* data)
{
    (void) data;

    /* Write the CTF packet data to the pkt */
    bctf_close_packet(&g_barectf_stream_ctx);

    /* Commit the pkt to the pktring */
    pktring_commit();
}

void barectf_platform_pktring_init(uint8_t* pktring_buffer)
{
    TRACE_ASSERT(pktring_buffer != NULL);
    TRACE_ALLOC_CRITICAL_SECTION();

    if(barectf_platform_pktring_is_enabled() == 0)
    {
        TRACE_ENTER_CRITICAL_SECTION();

        struct barectf_platform_callbacks cbs =
        {
            .TRACE_CAT3(TRACE_CFG_CLOCK_TYPE, _clock_, get_value) = get_clock,
            .is_backend_full = is_backend_full,
            .open_packet = open_packet,
            .close_packet = close_packet,
        };

        pktring_init(pktring_buffer);

        /* Initial buffer provided in the following open_packet() call */
        barectf_init(&g_barectf_stream_ctx, NULL, TRACE_CFG_PACKET_SIZE, cbs, NULL);

        open_packet(NULL);

        TRACE_EXIT_CRITICAL_SECTION();
    }
}

void barectf_platform_pktring_fini(void)
{
    TRACE_ALLOC_CRITICAL_SECTION();

    if(barectf_platform_pktring_is_enabled() != 0)
    {
        TRACE_ENTER_CRITICAL_SECTION();

        /* Close last packet if it contains at least one event */
        const int pkt_is_open = barectf_packet_is_open(&g_barectf_stream_ctx);
        const int pkt_is_empty = barectf_packet_is_empty(&g_barectf_stream_ctx);
        if(pkt_is_open && !pkt_is_empty)
        {
            close_packet(NULL);
        }

        barectf_enable_tracing(&g_barectf_stream_ctx, 0);

        TRACE_EXIT_CRITICAL_SECTION();
    }
}

int barectf_platform_pktring_is_enabled(void)
{
    return barectf_is_tracing_enabled(&g_barectf_stream_ctx);
}

void barectf_platform_pktring_flush(void)
{
    TRACE_ALLOC_CRITICAL_SECTION();

    if(barectf_platform_pktring_is_enabled() != 0)
    {
        TRACE_ENTER_CRITICAL_SECTION();

        /* If the last packet has data, flush it to the pktring and open a new one */
        const int pkt_is_open = barectf_packet_is_open(&g_barectf_stream_ctx);
        const int pkt_is_empty = barectf_packet_is_empty(&g_barectf_stream_ctx);
        if(pkt_is_open && !pkt_is_empty)
        {
            close_packet(NULL);
            open_packet(NULL);
        }

        TRACE_EXIT_CRITICAL_SECTION();
    }
}

int barectf_platform_pktring_next_packet(barectf_platform_pktring_packet_callback callback)
{
    TRACE_ASSERT(callback != NULL);

    int got_pkt = 0;
    TRACE_ALLOC_CRITICAL_SECTION();

    TRACE_ENTER_CRITICAL_SECTION();

    const uint8_t* pkt = pktring_read();
    if(pkt != NULL)
    {
        got_pkt = 1;
        callback(pkt);
        pktring_release();
    }

    TRACE_EXIT_CRITICAL_SECTION();

    return got_pkt;
}

barectf_stream_ctx* barectf_platform_pktring_ctx(void)
{
    return &g_barectf_stream_ctx;
}

/* pktring API */
API_VIZ void pktring_init(uint8_t* pktring_buffer)
{
    /* Ensure ring length is a power of 2 */
    TRACE_ASSERT(
            (TRACE_CFG_NUM_PACKETS != 0) && ((TRACE_CFG_NUM_PACKETS & (TRACE_CFG_NUM_PACKETS - 1)) == 0));

    g_barectf_pktring.len = 0;
    g_barectf_pktring.write_at = 0;
    g_barectf_pktring.read_from = 0;
    g_barectf_pktring_mem = pktring_buffer;
}

API_VIZ uint16_t pktring_length(void)
{
    return g_barectf_pktring.len;
}

API_VIZ uint8_t* pktring_grant(void)
{
    if((g_barectf_pktring.len != 0) && (g_barectf_pktring.write_at == g_barectf_pktring.read_from))
    {
        /* This grant is going to overwrite the tail */
        g_barectf_pktring.read_from = (g_barectf_pktring.read_from + 1) & PKTRING_LEN_MASK;
    }
    return &g_barectf_pktring_mem[g_barectf_pktring.write_at * TRACE_CFG_PACKET_SIZE];
}

API_VIZ void pktring_commit(void)
{
    if(g_barectf_pktring.len != TRACE_CFG_NUM_PACKETS)
    {
        g_barectf_pktring.len += 1;
    }
    g_barectf_pktring.write_at = (g_barectf_pktring.write_at + 1) & PKTRING_LEN_MASK;
}

API_VIZ const uint8_t* pktring_read(void)
{
    if(g_barectf_pktring.len == 0)
    {
        return NULL;
    }
    else
    {
        const uint8_t* ptr = &g_barectf_pktring_mem[g_barectf_pktring.read_from * TRACE_CFG_PACKET_SIZE];
        return ptr;
    }
}

API_VIZ void pktring_release(void)
{
    if(g_barectf_pktring.len != 0)
    {
        g_barectf_pktring.len -= 1;
        g_barectf_pktring.read_from = (g_barectf_pktring.read_from + 1) & PKTRING_LEN_MASK;
    }
}

#else
/* Prevent diagnostic warning about empty translation unit when tracing is disabled */
typedef int make_the_compiler_happy;
#endif /* defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1) */
