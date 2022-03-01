#ifndef TRACE_CONFIG_H
#define TRACE_CONFIG_H

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_CFG_TRACING_ENABLED (1)

/* Must match the barectf default clock $c-type field */
#define TRACE_CFG_CLOCK_C_TYPE uint64_t

/* Can optionally supply a packet context field if the stream type uses one */
#define TRACE_CFG_PACKET_CONTEXT_FIELD MY_NODE_ID

#define TRACE_CFG_PACKET_SIZE (64)

/* NOTE: must be a power of 2 */
#define TRACE_CFG_NUM_PACKETS (8)

#define TRACE_CFG_USE_TRACE_ASSERT (1)
#define TRACE_ASSERT(eval) assert(eval)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TRACE_CONFIG_H */
