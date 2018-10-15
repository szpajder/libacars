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

#include <stdint.h>

typedef struct {
	uint8_t *buf;
	uint32_t start;
	uint32_t end;
	uint32_t len;
} bitstream_t;

bitstream_t *bitstream_init(uint32_t len);
void bitstream_destroy(bitstream_t *bs);
int bitstream_append_msbfirst(bitstream_t *bs, uint8_t const *v, uint32_t const numbytes, uint32_t const numbits);
int bitstream_read_word_msbfirst(bitstream_t *bs, uint32_t *ret, uint32_t const numbits);
