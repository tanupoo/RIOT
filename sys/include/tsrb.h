#ifndef TSRB_H
#define TSRB_H

#include <stddef.h>

/**
 * @brief     Ringbuffer.
 * @details   Thread-safe FIFO tsrb implementation around a `char` array.
 */
typedef struct tsrb {
    char *buf;                  /**< Buffer to operate on. */
    unsigned int size;          /**< Size of buf. */
    volatile unsigned reads;    /**< total number of reads */
    volatile unsigned writes;   /**< total number of writes */
} tsrb_t;

#define TSRB_INIT(BUF) { (BUF), sizeof (BUF), 0, 0 }

/**
 * @brief        Initialize a tsrb.
 * @param[out]   rb        Datum to initialize.
 * @param[in]    buffer    Buffer to use by rb.
 * @param[in]    bufsize   `sizeof (buffer)`
 */
static inline void tsrb_init(tsrb_t *rb, char *buffer, unsigned bufsize)
{
    rb->buf = buffer;
    rb->size = bufsize;
    rb->reads = 0;
    rb->writes = 0;
}

/**
 * @brief           Test if the tsrb is empty.
 * @param[in,out]   rb  Ringbuffer to operate on.
 * @returns         0   if not empty
 */
static inline int tsrb_empty(const tsrb_t *rb)
{
    return (rb->reads == rb->writes);
}

static inline unsigned int tsrb_avail(const tsrb_t *rb)
{
    return (rb->writes - rb->reads);
}

static inline int tsrb_full(const tsrb_t *rb)
{
    return (rb->writes - rb->reads) == rb->size;
}

static inline unsigned int tsrb_free(const tsrb_t *rb)
{
    return (rb->size - rb->writes + rb->reads);
}

static inline void _push(tsrb_t *rb, char c)
{
    rb->buf[rb->writes++ & ( rb->size - 1)] = c;
}

static inline char _pop(tsrb_t *rb)
{
    return rb->buf[rb->reads++ & ( rb->size - 1)];
}

int tsrb_get_one(tsrb_t *rb);
int tsrb_get(tsrb_t *rb, char *dst, size_t n);
int tsrb_add_one(tsrb_t *rb, char c);
int tsrb_add(tsrb_t *rb, const char *src, size_t n);

#endif /* TSRB_H */
