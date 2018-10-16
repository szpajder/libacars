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

#ifndef LA_ARINC_H
#define LA_ARINC_H 1

#include <stdint.h>
#include "libacars.h"		// la_type_descriptor, la_proto_node
#include "vstring.h"		// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ARINC_MSG_UNKNOWN = 0,
	ARINC_MSG_CR1,
	ARINC_MSG_CC1,
	ARINC_MSG_DR1,
	ARINC_MSG_AT1,
	ARINC_MSG_ADS,
	ARINC_MSG_DIS
} la_arinc_imi;
#define LA_ARINC_IMI_CNT 7

typedef struct {
	char gs_addr[8];
	char air_reg[8];
	la_arinc_imi imi;
	bool crc_ok;
} la_arinc_msg;

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir const msg_dir);
void la_arinc_format_text(la_vstring * const vstr, void const * const data);
extern la_type_descriptor const la_DEF_arinc_message;

#ifdef __cplusplus
}
#endif

#endif // !LA_ARINC_H
