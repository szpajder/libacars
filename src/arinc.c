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

#include <stdbool.h>
#include <ctype.h>		// isupper(), isdigit()
#include <string.h>		// strstr()
#include "arinc.h"		// la_arinc_msg, LA_ARINC_IMI_CNT
#include "macros.h"		// la_debug_print
#include "vstring.h"		// la_vstring_append_sprintf
#include "util.h"		// la_slurp_hexstring
#include "adsc.h"		// adsc_parse()
#include "cpdlc.h"		// la_cpdlc_parse()

typedef enum {
	ARINC_APP_TYPE_UNKNOWN = 0,
	ARINC_APP_TYPE_CHARACTER,
	ARINC_APP_TYPE_BINARY
} la_arinc_app_type;

typedef struct {
	char const * const imi_string;
	la_arinc_imi imi;
} la_arinc_imi_map;

typedef struct {
	la_arinc_app_type app_type;
	char const * const description;
} la_arinc_imi_props;

static la_arinc_imi_map const imi_map[LA_ARINC_IMI_CNT] = {
	{ .imi_string = ".AT1", .imi = ARINC_MSG_AT1 },
	{ .imi_string = ".CR1", .imi = ARINC_MSG_CR1 },
	{ .imi_string = ".CC1", .imi = ARINC_MSG_CC1 },
	{ .imi_string = ".DR1", .imi = ARINC_MSG_DR1 },
	{ .imi_string = ".ADS", .imi = ARINC_MSG_ADS },
	{ .imi_string = ".DIS", .imi = ARINC_MSG_DIS },
	{ .imi_string = NULL,   .imi = ARINC_MSG_UNKNOWN }
};

static la_arinc_imi_props const imi_props[LA_ARINC_IMI_CNT] = {
	[ARINC_MSG_UNKNOWN]	= { .app_type = ARINC_APP_TYPE_UNKNOWN, .description = "Unknown message type"},
	[ARINC_MSG_AT1]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "CPDLC Message" },
	[ARINC_MSG_CR1]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "CPDLC Connect Request" },
	[ARINC_MSG_CC1]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "CPDLC Connect Confirm" },
	[ARINC_MSG_DR1]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "CPDLC Disconnect Request" },
	[ARINC_MSG_ADS]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "ADS-C message" },
	[ARINC_MSG_DIS]		= { .app_type = ARINC_APP_TYPE_BINARY,  .description = "ADS-C disconnect request" }
};

static bool is_numeric_or_uppercase(char const *str, size_t len) {
	if(!str) return false;
	for(int i = 0; i < len; i++) {
		if((!isupper(str[i]) && !isdigit(str[i])) || str[i] == '\0') {
			return false;
		}
	}
	return true;
}

static char *guess_arinc_msg_type(char const *txt, la_arinc_msg *msg) {
	if(txt == NULL) {
		return NULL;
	}
	la_assert(msg);
	la_arinc_imi imi = ARINC_MSG_UNKNOWN;
	char *imi_ptr = NULL;
	for(la_arinc_imi_map const *p = imi_map; ; p++) {
		if(p->imi_string == NULL) break;
		la_debug_print("Checking %s\n", p->imi_string);
		if((imi_ptr = strstr(txt, p->imi_string)) != NULL) {
			imi = p->imi;
			break;
		}
	}
	if(imi == ARINC_MSG_UNKNOWN) {
		la_debug_print("%s", "No known IMI found\n");
		return NULL;
	}
	char *gs_addr = NULL;
	size_t gs_addr_len = 0;
// Check for seven-character ground address (/AKLCDYA.AT1... or #MD/AA AKLCDYA.AT1...)
	if(imi_ptr - txt >= 8) {
		if(imi_ptr[-8] == '/' || imi_ptr[-8] == ' ') {
			if(is_numeric_or_uppercase(imi_ptr - 7, 7)) {
				gs_addr = imi_ptr - 7;
				gs_addr_len = 7;
				goto complete;
			}
		}
// Check for four-character ground address (/EDYY.AFN... or #M1B/B0 EDYY.AFN...)
	} else if(imi_ptr - txt >= 5) {
		if(imi_ptr[-5] == '/') {
			if(is_numeric_or_uppercase(imi_ptr - 4, 4)) {
				gs_addr = imi_ptr - 4;
				gs_addr_len = 4;
				goto complete;
			}
		}
	}
	la_debug_print("IMI %d found but no GS address\n", imi);
	return NULL;
complete:
	msg->imi = imi;
	memcpy(msg->gs_addr, gs_addr, gs_addr_len);
	msg->gs_addr[gs_addr_len] = '\0';
// Skip the dot before IMI and point to the start of the CRC-protected part
	return imi_ptr + 1;
}

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir const msg_dir) {
	if(txt == NULL) {
		return NULL;
	}
	la_arinc_msg *msg = LA_XCALLOC(1, sizeof(la_arinc_msg));
	la_proto_node *node = NULL;
	la_proto_node *next_node = NULL;

	char *payload = guess_arinc_msg_type(txt, msg);
	if(payload == NULL) {
		goto cleanup;
	}

	if(imi_props[msg->imi].app_type == ARINC_APP_TYPE_BINARY) {
		size_t payload_len = strlen(payload);
		if(payload_len < 3 + 7 + 4) {	// IMI + aircraft regnr + CRC
			la_debug_print("payload too short: %zu\n", payload_len);
			goto cleanup;
		}
		memcpy(msg->air_reg, payload + 3, 7);
		msg->air_reg[7] = '\0';
		la_debug_print("air_reg: %s\n", msg->air_reg);
// FIXME: compute CRC
		msg->crc_ok = true;
		payload += 10;
		uint8_t *buf = NULL;
		size_t buflen = la_slurp_hexstring(payload, &buf);
		buflen -= 2; // strip CRC
		switch(msg->imi) {
		case ARINC_MSG_CR1:
		case ARINC_MSG_CC1:
		case ARINC_MSG_DR1:
		case ARINC_MSG_AT1:
			next_node = la_cpdlc_parse(buf, buflen, msg_dir);
			LA_XFREE(buf);
			break;
		case ARINC_MSG_ADS:
		case ARINC_MSG_DIS:
			next_node = adsc_parse(buf, buflen, msg_dir, msg->imi);
			LA_XFREE(buf);
			break;
		default:
			break;
		}
	}

	node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_arinc_message;
	node->next = next_node;
	return node;
cleanup:
	LA_XFREE(msg);
	LA_XFREE(node);
	return NULL;
}

void la_arinc_format_text(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_arinc_msg *, data);
	la_vstring_append_sprintf(vstr, "FANS-1/A %s%s:\n",
		imi_props[msg->imi].description, msg->crc_ok ? "" : "(CRC check failed)");
}

la_type_descriptor const la_DEF_arinc_message = {
	.header = NULL,
	.format_text = &la_arinc_format_text,
	.destroy = NULL
};
