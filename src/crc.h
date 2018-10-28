/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LA_CRC_ARINC_H
#define LA_CRC_ARINC_H

#include <stdint.h>
#include <stdbool.h>

bool la_check_crc16_arinc(uint8_t const *data, uint32_t len);
uint16_t la_crc16_ccitt(uint8_t const *data, uint32_t len, uint16_t const crc_init);

#endif // !LA_CRC_ARINC_H
