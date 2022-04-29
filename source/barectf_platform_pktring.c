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

#define DEBUG_MARKER0 (0xF1F1)
#define DEBUG_MARKER1 (0xF2F2)
#define DEBUG_MARKER2 (0xF3F3)
#define DEBUG_MARKER3 (0xF3F3)

/* Simple ring buffer for CTF packets, starts with 0xB11BF11F, ends with ~0xB11BF11F */
#define PKTRING_LEN_MASK (TRACE_CFG_NUM_PACKETS - 1)
#define PKTRING_MASK_INDEX(i) (i & PKTRING_LEN_MASK)
typedef struct
{
    /* 0xB11BF11F */
    volatile uint8_t marker0;
    volatile uint8_t marker1;
    volatile uint8_t marker2;
    volatile uint8_t marker3;

    uint16_t write_at;
    uint16_t overwrite;
    uint16_t grant_active;
    uint16_t read_from;

    uint16_t debug_marker0; /* DEBUG_MARKER0 */
    uint16_t debug_marker1; /* DEBUG_MARKER1 */
    uint8_t *pktring_mem;
    uint16_t debug_marker2; /* DEBUG_MARKER2 */
    uint16_t debug_marker3; /* DEBUG_MARKER3 */

    /* ~0xB11BF11F == 0x4EE40EE0 */
    uint8_t marker4;
    uint8_t marker5;
    uint8_t marker6;
    uint8_t marker7;
} pktring_s;

static barectf_stream_ctx g_barectf_stream_ctx;
static pktring_s g_barectf_pktring;

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
static inline uint16_t max16(const uint16_t a, const uint16_t b)
{
    return a > b ? a : b;
}

API_VIZ void pktring_init(uint8_t* pktring_buffer)
{
    /* Ensure ring length is a power of 2 */
    TRACE_ASSERT(
            (TRACE_CFG_NUM_PACKETS != 0) && ((TRACE_CFG_NUM_PACKETS & (TRACE_CFG_NUM_PACKETS - 1)) == 0));

    g_barectf_pktring.marker4 = 0xE0;
    g_barectf_pktring.marker5 = 0x0E;
    g_barectf_pktring.marker6 = 0xE4;
    g_barectf_pktring.marker7 = 0x4E;

    g_barectf_pktring.write_at = 0;
    g_barectf_pktring.overwrite = 0;
    g_barectf_pktring.grant_active = 0;
    g_barectf_pktring.read_from = 0;
    g_barectf_pktring.debug_marker0 = DEBUG_MARKER0;
    g_barectf_pktring.debug_marker1 = DEBUG_MARKER1;
    g_barectf_pktring.pktring_mem = pktring_buffer;
    g_barectf_pktring.debug_marker2 = DEBUG_MARKER2;
    g_barectf_pktring.debug_marker3 = DEBUG_MARKER3;

    g_barectf_pktring.marker0 = 0x1F;
    g_barectf_pktring.marker1 = 0xF1;
    g_barectf_pktring.marker2 = 0x1B;
    g_barectf_pktring.marker3 = 0xB1;
}

API_VIZ uint16_t pktring_length(void)
{
    return g_barectf_pktring.write_at - max16(g_barectf_pktring.read_from, g_barectf_pktring.overwrite);
}

API_VIZ uint8_t* pktring_grant(void)
{
    g_barectf_pktring.grant_active = 1;
    if(g_barectf_pktring.write_at == (g_barectf_pktring.overwrite + TRACE_CFG_NUM_PACKETS))
    {
        g_barectf_pktring.overwrite += 1;
    }
    return &g_barectf_pktring.pktring_mem[PKTRING_MASK_INDEX(g_barectf_pktring.write_at) * TRACE_CFG_PACKET_SIZE];
}

API_VIZ void pktring_commit(void)
{
    g_barectf_pktring.write_at += 1;
    g_barectf_pktring.grant_active = 0;
}

API_VIZ const uint8_t* pktring_read(void)
{
    if(g_barectf_pktring.write_at == g_barectf_pktring.read_from)
    {
        return NULL;
    }
    else
    {
        g_barectf_pktring.read_from = max16(g_barectf_pktring.read_from, g_barectf_pktring.overwrite);
        const uint8_t* ptr = &g_barectf_pktring.pktring_mem[PKTRING_MASK_INDEX(g_barectf_pktring.read_from) * TRACE_CFG_PACKET_SIZE];
        return ptr;
    }
}

API_VIZ void pktring_release(void)
{
    if(g_barectf_pktring.write_at != g_barectf_pktring.read_from)
    {
        g_barectf_pktring.read_from = max16(g_barectf_pktring.read_from + 1, g_barectf_pktring.overwrite + 1);
    }
}

#else
/* Prevent diagnostic warning about empty translation unit when tracing is disabled */
typedef int make_the_compiler_happy;
#endif /* defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1) */
