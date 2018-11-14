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

#include <string.h>				// memcpy()
#include <libacars/libacars.h>			// la_proto_node
#include <libacars/macros.h>			// la_assert()
#include <libacars/arinc.h>			// la_arinc_parse()
#include <libacars/crc.h>			// la_crc16_ccitt()
#include <libacars/vstring.h>			// la_vstring, LA_ISPRINTF()
#include <libacars/util.h>			// la_debug_print(), LA_CAST_PTR()
#include <libacars/acars.h>

#define LA_ACARS_MIN_LEN	16		// including CRC and DEL
#define DEL 0x7f
#define ETX 0x03

static la_proto_node *la_try_acars_apps(la_acars_msg const * const msg, la_msg_dir const msg_dir) {
	la_proto_node *ret = NULL;
	switch(msg->label[0]) {
	case 'A':
		switch(msg->label[1]) {
		case '6':
		case 'A':
			if((ret = la_arinc_parse(msg->txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'B':
		switch(msg->label[1]) {
		case '6':
		case 'A':
			if((ret = la_arinc_parse(msg->txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'H':
		switch(msg->label[1]) {
		case '1':
			if((ret = la_arinc_parse(msg->txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	}
end:
	return ret;
}

la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir const msg_dir) {
	if(buf == NULL) {
		return NULL;
	}

	la_proto_node *node = la_proto_node_new();
	la_acars_msg *msg = LA_XCALLOC(1, sizeof(la_acars_msg));
	node->data = msg;
	node->td = &la_DEF_acars_message;
	uint8_t *buf2 = LA_XCALLOC(len, sizeof(uint8_t));

	if(len < LA_ACARS_MIN_LEN) {
		la_debug_print("too short: %u < %u\n", len, LA_ACARS_MIN_LEN);
		goto fail;
	}

	if(buf[len-1] != DEL) {
		la_debug_print("%02x: no DEL byte at end\n", buf[len-1]);
		goto fail;
	}
	len--;

	uint16_t crc = la_crc16_ccitt(buf, len, 0);
	la_debug_print("CRC check result: %04x\n", crc);
	len -= 3;
	msg->crc_ok = (crc == 0);

	int i = 0;
	for(i = 0; i < len; i++) {
		buf2[i] = buf[i] & 0x7f;
	}
	la_debug_print_buf_hex(buf2, len, "%s", "Msg data after parity bits removal:\n");

	int k = 0;
	msg->mode = buf2[k++];

	for (i = 0; i < 7; i++, k++) {
		msg->reg[i] = buf2[k];
	}
	msg->reg[7] = '\0';

	/* ACK/NAK */
	msg->ack = buf2[k++];
	if (msg->ack == 0x15)
		msg->ack = '!';

	msg->label[0] = buf2[k++];
	msg->label[1] = buf2[k++];
	if (msg->label[1] == 0x7f)
		msg->label[1] = 'd';
	msg->label[2] = '\0';

	msg->block_id = buf2[k++];
	if (msg->block_id == 0)
		msg->block_id = ' ';

	/* txt start  */
	msg->bs = buf2[k++];

	msg->no[0] = '\0';
	msg->flight_id[0] = '\0';

	if(k >= len || msg->bs == ETX) {	// empty message text
		msg->txt = strdup("");
		goto end;
	}

	if (msg->mode <= 'Z' && msg->block_id <= '9') {
		/* message no */
		for (i = 0; i < 4 && k < len; i++, k++) {
			msg->no[i] = buf2[k];
		}
		msg->no[i] = '\0';

		/* Flight id */
		for (i = 0; i < 6 && k < len; i++, k++) {
			msg->flight_id[i] = buf2[k];
		}
		msg->flight_id[i] = '\0';
	}

	len -= k;
	msg->txt = LA_XCALLOC(len + 1, sizeof(char));
	msg->txt[len] = '\0';
	if(len > 0) {
		memcpy(msg->txt, buf2 + k, len);
// Replace NULLs in text to make it printable
		for(i = 0; i < len; i++) {
			if(msg->txt[i] == 0)
				msg->txt[i] = '.';
		}
		node->next = la_try_acars_apps(msg, msg_dir);
	}
	goto end;
fail:
	msg->err = 1;
end:
	LA_XFREE(buf2);
	return node;
}

void la_acars_format_text(la_vstring *vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_acars_msg *, data);
	if(msg->err) {
		LA_ISPRINTF(vstr, indent, "%s", "-- Unparseable ACARS message\n");
		return;
	}
	LA_ISPRINTF(vstr, indent, "ACARS%s:\n", msg->crc_ok ? "" : " (warning: CRC error)");
	indent++;

	if(msg->mode < 0x5d) {		// air-to-ground
		LA_ISPRINTF(vstr, indent, "Reg: %s Flight: %s\n", msg->reg, msg->flight_id);
	}
	LA_ISPRINTF(vstr, indent, "Mode: %1c Label: %s Blk id: %c Ack: %c Msg no.: %s\n",
		msg->mode, msg->label, msg->block_id, msg->ack, msg->no);
	LA_ISPRINTF(vstr, indent, "%s\n", "Message:");
	LA_ISPRINTF(vstr, indent+1, "%s\n", msg->txt);
}

void la_acars_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	LA_CAST_PTR(msg, la_acars_msg *, data);
	LA_XFREE(msg->txt);
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_acars_message = {
	.header = NULL,
	.format_text = la_acars_format_text,
	.destroy = la_acars_destroy
};
