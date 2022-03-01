#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t util_timestamp_ns(void);

size_t util_drain_pktring_to_file(void);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H */
