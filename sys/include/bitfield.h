#ifndef BITFIELD_H
#define BITFIELD_H

#include <stdint.h>
#include <stdbool.h>

/* SIZE should be a constant expression
 * this avoids VLAs and problems resulting from being evaluated twice
 */
#define BITFIELD(NAME, SIZE) \
     uint8_t NAME[(SIZE) / 8 + ((SIZE) % 8 != 0)]

/**
 * @brief   Set the bit to 1
 *
 * @param[in,out] field The bitfield
 * @param[in]     idx   The number of the bit to set
 */
static inline void bf_set(uint8_t field[], size_t idx)
{
    field[idx / 8] |= 1u << (idx % 8);
}

/**
 * @brief   Clear the bit
 *
 * @param[in,out] field The bitfield
 * @param[in]     idx   The number of the bit to clear
 */
static inline void bf_unset(uint8_t field[], size_t idx)
{
    field[idx / 8] &= ~(1u << (idx % 8));
}

/**
 * @brief   Toggle the bit
 *
 * @param[in,out] field The bitfield
 * @param[in]     idx   The number of the bit to toggle
 */
static inline void bf_toggle(uint8_t field[], size_t idx)
{
    field[idx / 8] ^= 1u << (idx % 8);
}

/**
 * @brief  Check if the bet is set
 *
 * @param[in,out] field The bitfield
 * @param[in]     idx   The number of the bit to check
 */
static inline bool bf_isset(uint8_t field[], size_t idx)
{
    return field[idx / 8] & (1u << (idx % 8));
}

#endif /* BITFIELD_H */
