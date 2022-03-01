#ifndef TRACE_CONFIG_H
#define TRACE_CONFIG_H

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_CFG_TRACING_ENABLED (1)
#define TRACE_CFG_CLOCK_TYPE hrclock
#define TRACE_CFG_CLOCK_C_TYPE uint32_t
#define TRACE_CFG_PACKET_SIZE (64)
#define TRACE_CFG_NUM_PACKETS (8)
#define TRACE_CFG_USE_TRACE_ASSERT (1)

#define TRACE_ASSERT(eval) assert(eval)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TRACE_CONFIG_H */
