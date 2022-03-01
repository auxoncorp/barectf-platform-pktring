#ifndef TRACE_CONFIG_H
#define TRACE_CONFIG_H

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_CFG_TRACING_ENABLED (1)
#define TRACE_CFG_CLOCK_C_TYPE uint64_t
#define TRACE_CFG_PACKET_SIZE (128)
#define TRACE_CFG_NUM_PACKETS (8)
#define TRACE_CFG_USE_TRACE_ASSERT (1)
#define TRACE_CFG_BUFFER_MODE_FILL (1)

#define TRACE_ASSERT(eval) assert(eval)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TRACE_CONFIG_H */
