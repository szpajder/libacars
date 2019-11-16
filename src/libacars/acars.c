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

#define LA_ACARS_MIN_LEN	16		// including CRC and DEL
#define DEL 0x7f
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
	len -= 2;
	msg->crc_ok = (crc == 0);

	int i = 0;
	for(i = 0; i < len; i++) {
		buf2[i] = buf[i] & 0x7f;
	}

	if(buf2[len-1] == ETX) {
		msg->final_block = true;
	} else if(buf2[len-1] == ETB) {
		msg->final_block = false;
	} else {
		la_debug_print("%02x: no ETX/ETB byte at end of text\n", buf2[len-1]);
		goto fail;
	}
	len--;

	int k = 0;
	msg->mode = buf2[k++];

	for (i = 0; i < 7; i++, k++) {
		msg->reg[i] = buf2[k];
	}
	msg->reg[7] = '\0';

	msg->ack = buf2[k++];
// change special values to something printable
	if (msg->ack == NAK) {
		msg->ack = '!';
	} else if(msg->ack == ACK) {
		msg->ack = '^';
	}

	msg->label[0] = buf2[k++];
	msg->label[1] = buf2[k++];
	if (msg->label[1] == 0x7f)
		msg->label[1] = 'd';
	msg->label[2] = '\0';

	msg->block_id = buf2[k++];
	if (msg->block_id == 0)
		msg->block_id = ' ';

	char txt_start = buf2[k++];

	msg->no[0] = '\0';
	msg->flight_id[0] = '\0';

	if(k >= len || txt_start == ETX || txt_start == ETB) {	// empty message text
		msg->txt = strdup("");
		goto end;
	}

	if (IS_DOWNLINK_BLK(msg->block_id)) {
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
// If the message direction is unknown, guess it using the block ID character.
		if(msg_dir == LA_MSG_DIR_UNKNOWN) {
			if(IS_DOWNLINK_BLK(msg->block_id)) {
		                msg_dir = LA_MSG_DIR_AIR2GND;
			} else {
				msg_dir = LA_MSG_DIR_GND2AIR;
			}
			la_debug_print("Assuming msg_dir=%d\n", msg_dir);
		}
		node->next = la_acars_decode_apps(msg->label, msg->txt, msg_dir);
	}
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
