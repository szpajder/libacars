/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <string.h>				// memcpy(), strdup()
#include <libacars/libacars.h>			// la_proto_node, la_proto_tree_find_protocol
#include <libacars/macros.h>			// la_assert()
#include <libacars/arinc.h>			// la_arinc_parse()
#include <libacars/media-adv.h>			// la_media_adv_parse()
#include <libacars/miam.h>			// la_miam_parse()
#include <libacars/crc.h>			// la_crc16_ccitt()
#include <libacars/vstring.h>			// la_vstring, LA_ISPRINTF()
#include <libacars/json.h>			// la_json_append_*()
#include <libacars/util.h>			// la_debug_print(), LA_CAST_PTR()
#include <libacars/acars.h>

#define LA_ACARS_PREAMBLE_LEN	16		// including CRC and DEL, not including SOH
#define DEL 0x7f
#define STX 0x02
#define ETX 0x03
#define ETB 0x17
#define ACK 0x06
#define NAK 0x15
#define IS_DOWNLINK_BLK(bid) ((bid) >= '0' && (bid) <= '9')

la_proto_node *la_acars_decode_apps(char const * const label,
char const * const txt, la_msg_dir const msg_dir) {
	la_proto_node *ret = NULL;
	if(label == NULL || txt == NULL) {
		goto end;
	}
	switch(label[0]) {
	case 'A':
		switch(label[1]) {
		case '6':
		case 'A':
			if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'B':
		switch(label[1]) {
		case '6':
		case 'A':
			if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'H':
		switch(label[1]) {
		case '1':
			if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
				goto end;
			}
			if((ret = la_miam_parse(label, txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'M':
		switch(label[1]) {
		case 'A':
			if((ret = la_miam_parse(label, txt, msg_dir)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	case 'S':
		switch(label[1]) {
		case 'A':
			if((ret = la_media_adv_parse(txt)) != NULL) {
				goto end;
			}
			break;
		}
		break;
	}
end:
	return ret;
}

// Note: buf must contain raw ACARS bytes, NOT including initial SOH byte
// (0x01) and including terminating DEL byte (0x7f).
la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir msg_dir) {
	if(buf == NULL) {
		return NULL;
	}

	la_proto_node *node = la_proto_node_new();
	LA_NEW(la_acars_msg, msg);
	node->data = msg;
	node->td = &la_DEF_acars_message;
	char *buf2 = LA_XCALLOC(len, sizeof(char));

	msg->err = false;
	if(len < LA_ACARS_PREAMBLE_LEN) {
		la_debug_print("Preamble too short: %u < %u\n", len, LA_ACARS_PREAMBLE_LEN);
		goto fail;
	}

	if(buf[len-1] != DEL) {
		la_debug_print("%02x: no DEL byte at end\n", buf[len-1]);
		goto fail;
	}
	len--;

	uint16_t crc = la_crc16_ccitt(buf, len, 0);
	la_debug_print("CRC check result: %04x\n", crc);
	len -= 2;
	msg->crc_ok = (crc == 0);

	int i = 0;
	for(i = 0; i < len; i++) {
		buf2[i] = buf[i] & 0x7f;
	}
	la_debug_print_buf_hex(buf2, len, "After CRC and parity bit removal:\n");
	la_debug_print("Length: %d\n", len);

	if(buf2[len-1] == ETX) {
		msg->final_block = true;
	} else if(buf2[len-1] == ETB) {
		msg->final_block = false;
	} else {
		la_debug_print("%02x: no ETX/ETB byte at end of text\n", buf2[len-1]);
		goto fail;
	}
	len--;
// Here we have DEL, CRC and ETX/ETB bytes removed.
// There at least 12 bytes remaining.

	int remaining = len;
	char *ptr = buf2;

	msg->mode = *ptr;
	ptr++; remaining--;

	memcpy(msg->reg, ptr, 7);
	msg->reg[7] = '\0';
	ptr += 7; remaining -= 7;

	msg->ack = *ptr;
	ptr++; remaining--;

// change special values to something printable
	if (msg->ack == NAK) {
		msg->ack = '!';
	} else if(msg->ack == ACK) {
		msg->ack = '^';
	}

	msg->label[0] = *ptr++;
	msg->label[1] = *ptr++;
	remaining -= 2;

	if (msg->label[1] == 0x7f) {
		msg->label[1] = 'd';
	}
	msg->label[2] = '\0';

	msg->block_id = *ptr;
	ptr++; remaining--;

	if (msg->block_id == 0) {
		msg->block_id = ' ';
	}
// If the message direction is unknown, guess it using the block ID character.
	if(msg_dir == LA_MSG_DIR_UNKNOWN) {
		if(IS_DOWNLINK_BLK(msg->block_id)) {
			msg_dir = LA_MSG_DIR_AIR2GND;
		} else {
			msg_dir = LA_MSG_DIR_GND2AIR;
		}
		la_debug_print("Assuming msg_dir=%d\n", msg_dir);
	}
	if(remaining < 1) {
// ACARS preamble has been consumed up to this point.
// If this is an uplink with an empty message text, then we are done.
		if(!IS_DOWNLINK_BLK(msg->block_id)) {
			msg->txt = strdup("");
			goto end;
		} else {
			la_debug_print("No text field in downlink message\n");
			goto fail;
		}
	}
// Otherwise we expect STX here.
	if(*ptr != STX) {
		la_debug_print("%02x: No STX byte after preamble\n", *ptr);
		goto fail;
	}
	ptr++; remaining--;

// Replace NULLs in message text to make it printable
// XXX: Should we replace all nonprintable chars here?
	for(i = 0; i < remaining; i++) {
		if(ptr[i] == 0) {
			ptr[i] = '.';
		}
	}
// Extract downlink-specific fields from message text
	if (IS_DOWNLINK_BLK(msg->block_id)) {
		if(remaining < 10) {
			la_debug_print("Downlink text field too short: %d < 10\n", remaining);
			goto fail;
		}
		memcpy(msg->no, ptr, 4);
		ptr += 4; remaining -= 4;
		memcpy(msg->flight_id, ptr, 6);
		ptr += 6; remaining -= 6;
	}

	msg->txt = LA_XCALLOC(remaining + 1, sizeof(char));
	if(remaining < 1) {
		goto end;
	}

	memcpy(msg->txt, ptr, remaining);
	node->next = la_acars_decode_apps(msg->label, msg->txt, msg_dir);
	goto end;
fail:
	msg->err = true;
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
		LA_ISPRINTF(vstr, indent, "-- Unparseable ACARS message\n");
		return;
	}
	LA_ISPRINTF(vstr, indent, "ACARS%s:\n", msg->crc_ok ? "" : " (warning: CRC error)");
	indent++;

	LA_ISPRINTF(vstr, indent, "Reg: %s", msg->reg);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_vstring_append_sprintf(vstr, " Flight: %s\n", msg->flight_id);
	} else {
		la_vstring_append_sprintf(vstr, "%s", "\n");
	}

	LA_ISPRINTF(vstr, indent, "Mode: %1c Label: %s Blk id: %c More: %d Ack: %c",
		msg->mode, msg->label, msg->block_id, !msg->final_block, msg->ack);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_vstring_append_sprintf(vstr, " Msg no.: %s\n", msg->no);
	} else {
		la_vstring_append_sprintf(vstr, "%s", "\n");
	}

	LA_ISPRINTF(vstr, indent, "Message:\n");
	la_isprintf_multiline_text(vstr, indent+1, msg->txt);
}

void la_acars_format_json(la_vstring *vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_acars_msg *, data);
	la_json_append_bool(vstr, "err", msg->err);
	if(msg->err) {
		return;
	}
	la_json_append_bool(vstr, "crc_ok", msg->crc_ok);
	la_json_append_bool(vstr, "more", !msg->final_block);
	la_json_append_string(vstr, "reg", msg->reg);
	la_json_append_char(vstr, "mode", msg->mode);
	la_json_append_string(vstr, "label", msg->label);
	la_json_append_char(vstr, "blk_id", msg->block_id);
	la_json_append_char(vstr, "ack", msg->ack);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_json_append_string(vstr, "flight", msg->flight_id);
		la_json_append_string(vstr, "msg_no", msg->no);
	}
	la_json_append_string(vstr, "msg_text", msg->txt);
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
	.format_text = la_acars_format_text,
	.format_json = la_acars_format_json,
	.json_key = "acars",
	.destroy = la_acars_destroy
};

la_proto_node *la_proto_tree_find_acars(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_acars_message);
}
