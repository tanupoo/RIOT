/* packet-zep.h
 * Dissector  routines for the ZigBee Encapsulation Protocol
 * By Owen Kirby <osk@exegin.com>
 * Copyright 2009 Exegin Technologies Limited
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdbool.h>
#include <time.h>

#define ZEP_DEFAULT_PORT   17754

/*  ZEP Preamble Code */
#define ZEP_PREAMBLE        "EX"

/*  ZEP Header lengths. */
#define ZEP_V1_HEADER_LEN   16
#define ZEP_V2_HEADER_LEN   32
#define ZEP_V2_ACK_LEN      8

#define ZEP_V2_TYPE_DATA    1
#define ZEP_V2_TYPE_ACK     2

#define ZEP_LENGTH_MASK     0x7F

typedef struct{
    uint8_t     version;
    uint8_t     type;
    uint8_t     channel_id;
    uint16_t    device_id;
    bool        lqi_mode;
    uint8_t     lqi;
    time_t    ntp_time;
    uint32_t    seqno;
} zep_info;
