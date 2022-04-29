#ifndef BARECTF_PLATFORM_PKTRING_H
#define BARECTF_PLATFORM_PKTRING_H

#include <stdint.h>

#include "trace_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_NOOP_STATEMENT do {} while( 0 )

/* Size is always TRACE_CFG_PACKET_SIZE */
typedef void (*barectf_platform_pktring_packet_callback)(const uint8_t* packet);

#if defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1)

#if defined(TRACE_CFG_USE_CRITICAL_SECTIONS)
#if !defined(TRACE_CFG_ALLOC_CRITICAL_SECTION) || !defined(TRACE_CFG_ENTER_CRITICAL_SECTION) || !defined(TRACE_CFG_EXIT_CRITICAL_SECTION)
    #error "Missing critical section definition. Define TRACE_CFG_ALLOC_CRITICAL_SECTION, TRACE_CFG_ENTER_CRITICAL_SECTION, and TRACE_CFG_EXIT_CRITICAL_SECTION in trace_config.h for your target."
#else
    #define TRACE_ALLOC_CRITICAL_SECTION() TRACE_CFG_ALLOC_CRITICAL_SECTION()
    #define TRACE_ENTER_CRITICAL_SECTION() TRACE_CFG_ENTER_CRITICAL_SECTION()
    #define TRACE_EXIT_CRITICAL_SECTION() TRACE_CFG_EXIT_CRITICAL_SECTION()
#endif
#else
    #define TRACE_ALLOC_CRITICAL_SECTION() TRACE_NOOP_STATEMENT
    #define TRACE_ENTER_CRITICAL_SECTION() TRACE_NOOP_STATEMENT
    #define TRACE_EXIT_CRITICAL_SECTION() TRACE_NOOP_STATEMENT
#endif /* defined(TRACE_CFG_USE_CRITICAL_SECTIONS) */

/* A macro to wrap calling the barectf generated tracing functions in a critical section if enabled.
 * Also useful for turning tracing calls into no-op statements based on TRACE_CFG_TRACING_ENABLED */
#if defined(TRACE_CFG_USE_CRITICAL_SECTIONS)
#define TRACE(barectf_trace_call) \
    { \
        TRACE_ALLOC_CRITICAL_SECTION(); \
        TRACE_ENTER_CRITICAL_SECTION(); \
        barectf_trace_call; \
        TRACE_EXIT_CRITICAL_SECTION(); \
    }
#else
#define TRACE(barectf_trace_call) \
    { \
        barectf_trace_call; \
    }
#endif /* defined(TRACE_CFG_USE_CRITICAL_SECTIONS) */


#if !defined(TRACE_CFG_CLOCK_C_TYPE)
#error "TRACE_CFG_CLOCK_C_TYPE must be defined to match the barectf default clock $c-type"
#endif

#if defined(TRACE_CFG_BUFFER_MODE_FILL) && defined(TRACE_CFG_BUFFER_MODE_OVERWRITE)
#error "Both TRACE_CFG_BUFFER_MODE_FILL and TRACE_CFG_BUFFER_MODE_OVERWRITE buffer modes cannot be defined together"
#endif

/* The default buffer mode is overwrite, pktring will drop older data for more recent data */
#if !defined(TRACE_CFG_BUFFER_MODE_FILL) && !defined(TRACE_CFG_BUFFER_MODE_OVERWRITE)
#define TRACE_CFG_BUFFER_MODE_OVERWRITE (1)
#endif

#if !defined(TRACE_CFG_PACKET_SIZE) || (TRACE_CFG_PACKET_SIZE <= 0)
#error "TRACE_CFG_PACKET_SIZE must be defined and greater than zero"
#endif

#if !defined(TRACE_CFG_NUM_PACKETS) || (TRACE_CFG_NUM_PACKETS <= 0)
#error "TRACE_CFG_NUM_PACKETS must be defined and greater than zero"
#endif

#if (TRACE_CFG_NUM_PACKETS > 32768)
#error "TRACE_CFG_NUM_PACKETS is too big, it must be less than or equal to 2^15"
#endif

/* Size in bytes of the pktring memory buffer, allocated by the user */
#define TRACE_PKTRING_BUFFER_SIZE (TRACE_CFG_PACKET_SIZE * TRACE_CFG_NUM_PACKETS)

#if defined(TRACE_CFG_USE_TRACE_ASSERT) && (TRACE_CFG_USE_TRACE_ASSERT != 0)
#if !defined(TRACE_ASSERT)
#error "TRACE_ASSERT must be defined when TRACE_CFG_USE_TRACE_ASSERT != 0"
#endif
#else
#define TRACE_ASSERT(eval) TRACE_NOOP_STATEMENT
#endif /* defined(TRACE_CFG_USE_TRACE_ASSERT) && (TRACE_CFG_USE_TRACE_ASSERT != 0) */

#if !defined(TRACE_CAT3)
#define TRACE__CAT3(a, b, c) a##b##c
#define TRACE_CAT3(a, b, c) TRACE__CAT3(a, b, c)
#endif /* !defined(TRACE_CAT3) */

/* Use the 'default' stream type if TRACE_CFG_STREAM_TYPE is not specified */
#if !defined(TRACE_CFG_STREAM_TYPE)
#define TRACE_CFG_STREAM_TYPE default
#endif
typedef struct TRACE_CAT3(barectf_, TRACE_CFG_STREAM_TYPE, _ctx) barectf_stream_ctx;

/* Users must implement this fn */
extern TRACE_CFG_CLOCK_C_TYPE barectf_platform_pktring_get_clock(void);

/* Must ensure the clock source is configured prior to calling. */
void barectf_platform_pktring_init(uint8_t* pktring_buffer);

void barectf_platform_pktring_fini(void);

/* A wrapper to barectf_is_tracing_enabled */
int barectf_platform_pktring_is_enabled(void);

/* Write the currently open packet, if any, to the pktring. */
void barectf_platform_pktring_flush(void);

/* Get the next available CTF packet from the pktring, if any. Returns zero if none. */
int barectf_platform_pktring_next_packet(barectf_platform_pktring_packet_callback callback);

barectf_stream_ctx* barectf_platform_pktring_ctx(void);

#else /* defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1) */

#if !defined(TRACE_CFG_PACKET_SIZE)
#define TRACE_CFG_PACKET_SIZE (0)
#endif
#if !defined(TRACE_CFG_NUM_PACKETS)
#define TRACE_CFG_NUM_PACKETS (0)
#endif

#define TRACE_PKTRING_BUFFER_SIZE (0)
typedef int barectf_stream_ctx;
#define barectf_platform_pktring_init(x) TRACE_NOOP_STATEMENT
#define barectf_platform_pktring_fini() TRACE_NOOP_STATEMENT
#define barectf_platform_pktring_is_enabled() 0
#define barectf_platform_pktring_flush() TRACE_NOOP_STATEMENT
#define barectf_platform_pktring_next_packet(x) 0
#define barectf_platform_pktring_ctx() (NULL)

#define TRACE(barectf_trace_call) TRACE_NOOP_STATEMENT

#endif /* defined(TRACE_CFG_TRACING_ENABLED) && (TRACE_CFG_TRACING_ENABLED == 1) */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BARECTF_PLATFORM_PKTRING_H */
