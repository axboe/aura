/* SPDX-License-Identifier: MIT */
/*
 * EXPERIMENT. The trace ring sent out on the DockChannel by aura itself, in frames a host only
 * has to listen to (src/sptm_virt_s2r.c: sptm_virt_s2r_stream). Shared with the host test that
 * builds this header natively, so it uses nothing of aura's.
 *
 * A frame: this header, len payload bytes, then the CRC-32 (IEEE, reflected) of both.
 */
#ifndef S2R_STREAM_H
#define S2R_STREAM_H

#include <stdint.h>

#define S2R_STREAM_MAGIC 0x46523253u /* "S2RF" */
#define S2R_STREAM_VER   1
#define S2R_STREAM_CHUNK 256 /* records in an 'R' frame */

#define S2R_FRAME_HEARTBEAT                                                                        \
    'H'                       /* a: count, b: 1 once the sleep rules are armed; payload: head, t */
#define S2R_FRAME_INFO    'I' /* a: pass; payload: struct s2r_stream_info */
#define S2R_FRAME_RECORDS 'R' /* a: pass, b/c: sequence number of the first record (low, high) */
#define S2R_FRAME_END     'E' /* a: pass, b: record frames sent in it, c: 1 for a whole-ring pass */

struct s2r_stream_hdr {
    uint32_t magic;
    uint8_t type;
    uint8_t ver;
    uint16_t len;
    uint32_t a, b, c;
};

struct s2r_stream_info {
    uint64_t head;     /* records appended since boot; the newest is head - 1 */
    uint64_t nrec;     /* the ring's capacity in records */
    uint64_t cntfrq;   /* Hz of the records' timestamps */
    uint64_t t_stop;   /* CNTPCT_EL0 at the withheld commit */
    uint64_t stop_pa;  /* the commit register */
    uint64_t stop_val; /* the value XNU wrote, not performed */
    uint64_t pc;       /* XNU's PC at that write */
    uint32_t cpu;      /* the core that wrote it */
    uint32_t pages;    /* stage-2 pages the trace hooks */
    uint32_t quick;    /* record frames in a quick pass */
    uint32_t rec_size; /* sizeof(struct s2r_rec) */
    char build[48];    /* aura's version string */
};

static inline uint32_t s2r_stream_crc(uint32_t crc, const void *buf, uint32_t len)
{
    const uint8_t *p = buf;

    crc = ~crc;
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xedb88320u & -(crc & 1));
    }
    return ~crc;
}

/* Fills the header for a payload of len bytes and returns the frame's CRC. */
static inline uint32_t s2r_stream_frame(struct s2r_stream_hdr *h, uint8_t type, uint32_t a,
                                        uint32_t b, uint32_t c, const void *payload, uint16_t len)
{
    h->magic = S2R_STREAM_MAGIC;
    h->type = type;
    h->ver = S2R_STREAM_VER;
    h->len = len;
    h->a = a;
    h->b = b;
    h->c = c;
    return s2r_stream_crc(s2r_stream_crc(0, h, sizeof(*h)), payload, len);
}

#endif
