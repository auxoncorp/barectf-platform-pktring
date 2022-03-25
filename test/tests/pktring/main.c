#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "barectf_platform_pktring.h"
#include "barectf.h"
#include "util.h"

void pktring_init(uint8_t* pktring_buffer);
uint16_t pktring_length(void);
uint8_t* pktring_grant(void);
void pktring_commit(void);
const uint8_t* pktring_read(void);
void pktring_release(void);

static uint8_t g_pktring_mem[TRACE_PKTRING_BUFFER_SIZE] = { 0 };

uint64_t barectf_platform_pktring_get_clock(void)
{
    /* pktring doesn't use the clock */
    assert(0);
    return 0;
}

static void check_pkt(const uint8_t* pkt, uint8_t value)
{
    size_t i;
    assert(pkt != NULL);
    for(i = 0; i < TRACE_CFG_PACKET_SIZE; i += 1)
    {
        assert(pkt[i] == value);
    }
}

static void grant_commit_read_release(void)
{
    memset(g_pktring_mem, 0, TRACE_PKTRING_BUFFER_SIZE);
    pktring_init(g_pktring_mem);

    assert(pktring_length() == 0);

    uint8_t* wpkt = pktring_grant();
    memset(wpkt, 0x11, TRACE_CFG_PACKET_SIZE);
    assert(pktring_length() == 0);
    check_pkt(wpkt, 0x11);
    pktring_commit();
    assert(pktring_length() == 1);

    const uint8_t* rpkt = pktring_read();
    assert(rpkt != NULL);
    assert(pktring_length() == 1);
    check_pkt(rpkt, 0x11);
    pktring_release();
    assert(pktring_length() == 0);

    assert(pktring_read() == NULL);
    pktring_release();
    assert(pktring_length() == 0);
}

static void head_overwrites_tail(void)
{
    memset(g_pktring_mem, 0, TRACE_PKTRING_BUFFER_SIZE);
    pktring_init(g_pktring_mem);

    assert(pktring_length() == 0);
    assert(TRACE_CFG_NUM_PACKETS < (UINT8_MAX/2));

    /* Fill the ring */
    size_t i;
    for(i = 0; i < TRACE_CFG_NUM_PACKETS; i += 1)
    {
        uint8_t* wpkt = pktring_grant();
        memset(wpkt, i + 1, TRACE_CFG_PACKET_SIZE);
        check_pkt(wpkt, i + 1);
        /* Grant doesn't increment length */
        assert(pktring_length() == i);
        pktring_commit();
        /* Commit does increment length */
        assert(pktring_length() == (i + 1));
    }
    assert(pktring_length() == TRACE_CFG_NUM_PACKETS);

    /* Overwrite 2 more */
    for(i = 1; i <= 2; i += 1)
    {
        uint8_t* wpkt = pktring_grant();
        memset(wpkt, TRACE_CFG_NUM_PACKETS + i, TRACE_CFG_PACKET_SIZE);
        check_pkt(wpkt, TRACE_CFG_NUM_PACKETS + i);
        pktring_commit();
        assert(pktring_length() == TRACE_CFG_NUM_PACKETS);
    }

    /* Grants move the tail when full/overwritten */
    for(i = 0; i < TRACE_CFG_NUM_PACKETS - 2; i += 1)
    {
        const uint8_t* rpkt = pktring_read();
        assert(rpkt != NULL);
        /* Read doesn't decrement length */
        assert(pktring_length() == TRACE_CFG_NUM_PACKETS - i);
        check_pkt(rpkt, 3 + i);
        pktring_release();
        /* Release does decrement length */
        assert(pktring_length() == TRACE_CFG_NUM_PACKETS - i - 1);
    }

    /* Read the 2 overwritten entries that wrapped around */
    assert(pktring_length() == 2);
    for(i = 0; i < 2; i += 1)
    {
        const uint8_t* rpkt = pktring_read();
        assert(rpkt != NULL);
        assert(pktring_length() == 2 - i);
        check_pkt(rpkt, TRACE_CFG_NUM_PACKETS + 1 + i);
        pktring_release();
        assert(pktring_length() == 2 - i - 1);
    }
}

static void read_while_grant_not_committed(void)
{
    memset(g_pktring_mem, 0, TRACE_PKTRING_BUFFER_SIZE);
    pktring_init(g_pktring_mem);

    assert(pktring_length() == 0);

    /* Read is empty when first grant not committed */
    uint8_t* wpkt = pktring_grant();
    memset(wpkt, 1, TRACE_CFG_PACKET_SIZE);
    check_pkt(wpkt, 1);

    {
        assert(pktring_length() == 0);
        const uint8_t* rpkt = pktring_read();
        assert(rpkt == NULL);
    }

    pktring_commit();
    assert(pktring_length() == 1);

    const uint8_t* rpkt = pktring_read();
    assert(rpkt != NULL);
    check_pkt(rpkt, 1);
    pktring_release();
    assert(pktring_length() == 0);
}

static void grant_not_committed_moves_tail(void)
{
    memset(g_pktring_mem, 0, TRACE_PKTRING_BUFFER_SIZE);
    pktring_init(g_pktring_mem);

    assert(pktring_length() == 0);

    assert(TRACE_CFG_NUM_PACKETS < (UINT8_MAX/2));

    /* Fill the ring */
    size_t i;
    for(i = 0; i < TRACE_CFG_NUM_PACKETS; i += 1)
    {
        uint8_t* wpkt = pktring_grant();
        memset(wpkt, i + 1, TRACE_CFG_PACKET_SIZE);
        check_pkt(wpkt, i + 1);
        assert(pktring_length() == i);
        pktring_commit();
        assert(pktring_length() == (i + 1));
    }
    assert(pktring_length() == TRACE_CFG_NUM_PACKETS);

    /* Grant not committed moves the tail, next grant wraps to occupy the front */
    uint8_t* wpkt = pktring_grant();
    memset(wpkt, TRACE_CFG_NUM_PACKETS + 1, TRACE_CFG_PACKET_SIZE);
    check_pkt(wpkt, TRACE_CFG_NUM_PACKETS + 1);

    /* Tail is at the second position after the grant */
    const uint8_t* rpkt = pktring_read();
    assert(rpkt != NULL);
    check_pkt(rpkt, 2);
    pktring_release();
    assert(pktring_length() == TRACE_CFG_NUM_PACKETS - 2);

    /* Commit marks it occupied */
    pktring_commit();
    assert(pktring_length() == TRACE_CFG_NUM_PACKETS - 1);
}

static void full_circle(void)
{
    memset(g_pktring_mem, 0, TRACE_PKTRING_BUFFER_SIZE);
    pktring_init(g_pktring_mem);

    assert(pktring_length() == 0);

    assert(TRACE_CFG_NUM_PACKETS < (UINT8_MAX/4));

    size_t i;
    for(i = 0; i < (4 * TRACE_CFG_NUM_PACKETS); i += 1)
    {
        uint8_t* wpkt = pktring_grant();
        memset(wpkt, i + 1, TRACE_CFG_PACKET_SIZE);
        check_pkt(wpkt, i + 1);
        pktring_commit();
    }
    assert(pktring_length() == TRACE_CFG_NUM_PACKETS);

    for(i = 0; i < TRACE_CFG_NUM_PACKETS; i += 1)
    {
        const uint8_t* rpkt = pktring_read();
        assert(rpkt != NULL);
        assert(pktring_length() == TRACE_CFG_NUM_PACKETS - i);
        check_pkt(rpkt, (TRACE_CFG_NUM_PACKETS * 3) + 1 + i);
        pktring_release();
        assert(pktring_length() == TRACE_CFG_NUM_PACKETS - i - 1);
    }
    assert(pktring_length() == 0);
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    grant_commit_read_release();
    head_overwrites_tail();
    read_while_grant_not_committed();
    grant_not_committed_moves_tail();
    full_circle();

    return EXIT_SUCCESS;
}
