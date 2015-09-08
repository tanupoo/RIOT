/*
 * Copyright (C) 2015 Oliver Hahm <oliver.hahm@inria.fr>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup   board_msba2
 * @{
 *
 * @file
 * @brief     cc110x board specific configuration
 *
 * @author    Kaspar Schleiser <kaspar@schleiser.de>
 * @author    Oliver Hahm <oliver.hahm@inria.fr>
 */

#ifndef CC110X_PARAMS_H
#define CC110X_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name CC110X configuration
 */
const cc110x_params_t cc110x_params[] = {
    {
        .spi  = 1,
        .CS   = 6,
        .GDO0 = 70,
        .GDO1 = 8,
        .GDO2 = 28
    },
};
/** @} */

#ifdef __cplusplus
}
#endif
#endif /* CC110X_PARAMS_H */
/** @} */
