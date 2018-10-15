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
#include "util.h"		// LA_XCALLOC, LA_XFREE
#include "bitstream.h"		// bitstream_t

bitstream_t *bitstream_init(uint32_t len) {
	bitstream_t *ret;
	if(len == 0) return NULL;
	ret = LA_XCALLOC(1, sizeof(bitstream_t));
	ret->buf = LA_XCALLOC(len, sizeof(uint8_t));
	ret->start = ret->end = 0;
	ret->len = len;
	return ret;
}

void bitstream_destroy(bitstream_t *bs) {
	if(bs != NULL) LA_XFREE(bs->buf);
	LA_XFREE(bs);
}

int bitstream_append_msbfirst(bitstream_t *bs, uint8_t const *v, uint32_t const numbytes, uint32_t const numbits) {
	if(bs->end + numbits * numbytes > bs->len)
		return -1;
	for(int i = 0; i < numbytes; i++) {
		uint8_t t = v[i];
		for(int j = numbits - 1; j >= 0; j--)
			bs->buf[bs->end++] = (t >> j) & 0x01;
	}
	return 0;
}

int bitstream_read_word_msbfirst(bitstream_t *bs, uint32_t *ret, uint32_t const numbits) {
	if(bs->start + numbits > bs->end)
		return -1;
	*ret = 0;
	for(uint32_t i = 0; i < numbits; i++) {
		*ret |= (0x01 & bs->buf[bs->start++]) << (numbits-i-1);
	}
	return 0;
}
