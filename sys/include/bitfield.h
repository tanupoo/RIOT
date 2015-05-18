#ifndef BITFIELD_H
#define BITFIELD_H

#include <limits.h>
#include <stdbool.h>

/* SIZE should be a constant expression
 * this avoids VLAs and problems resulting from being evaluated twice
 */
 #define BITFIELD(SIZE, NAME) \
     unsigned char NAME[(SIZE) / CHAR_BIT + ((SIZE) % CHAR_BIT != 0)]

static inline void bf_set(unsigned char field[], size_t idx)
{
    field[idx / CHAR_BIT] |= 1u << (idx % CHAR_BIT);
}

static inline void bf_unset(unsigned char field[], size_t idx)
{
    field[idx / CHAR_BIT] &= ~(1u << (idx % CHAR_BIT));
}

static inline void bf_toggle(unsigned char field[], size_t idx)
{
    field[idx / CHAR_BIT] ^= 1u << (idx % CHAR_BIT);
}

static inline bool bf_isset(unsigned char field[], size_t idx)
{
    return field[idx / CHAR_BIT] & (1u << (idx % CHAR_BIT));
}

#endif /* BITFIELD_H */
