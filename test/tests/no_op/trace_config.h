#ifndef TRACE_CONFIG_H
#define TRACE_CONFIG_H

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_CFG_USE_TRACE_ASSERT (1)
#define TRACE_ASSERT(eval) assert(eval)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TRACE_CONFIG_H */
