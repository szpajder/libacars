/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include "config.h"                 // WITH_ZLIB, WITH_JANSSON
#include <string.h>                 // strlen
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>               // struct timeval
#endif
#ifdef WITH_JANSSON
#include <jansson.h>
#endif
#include <libacars/libacars.h>      // la_proto_node, la_type_descriptor
#include <libacars/reassembly.h>
#include <libacars/util.h>          // la_base64_decode
#include <libacars/macros.h>        // la_debug_print()
#include <libacars/ohma.h>          // la_ohma_msg

la_proto_node *la_ohma_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
#ifdef WITH_ZLIB
	if(txt == NULL) {
		return NULL;
	}
	size_t len = strlen(txt);
	// OHMA message recognition logic:
	// - short form: starts with "OHMA"
	// - long form: additionally preceded with '/', 7-char ground address and '.' (eg. "/RTNBOCR.OHMA")
	if(len >= 13 && txt[0] == '/' && txt[8] == '.') {
	    txt += 9; len -= 9;
	}
	if(strncmp(txt, "OHMA", 4) == 0) {
		txt += 4; len -= 4;
	} else {
	    return NULL;
	}

	la_octet_string *b64_decoded_msg = la_base64_decode(txt, len);
	if(b64_decoded_msg == NULL) {
		la_debug_print(D_INFO, "Failed to decode message as BASE64\n");
		// Fail silently without producing a node, since it's probably not an OHMA message
		return NULL;
	}

	LA_NEW(la_ohma_msg, msg);
	msg->err = LA_OHMA_SUCCESS;
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_ohma_msg;
	node->data = msg;
	node->next = NULL;

	// We need at least 3 octets (ZLIB CMF & FLG octets plus one octet of data).
	if(b64_decoded_msg->len < 3) {  
		la_debug_print(D_INFO, "Message too short: len %zu < min_len 3\n",
			b64_decoded_msg->len);
		msg->err = LA_OHMA_FAIL_MSG_TOO_SHORT;
		goto end;
	}
	// RFC1950 ZLIB compressed data stream should follow. CM=8 indicates DEFLATE compression.
	uint8_t cm = b64_decoded_msg->buf[0] & 0xf;
	if(cm != 8) {
	    la_debug_print(D_INFO, "Unknown compression algorithm ID: 0x%hu\n", cm);
	    msg->err = LA_OHMA_FAIL_UNKNOWN_COMPRESSION;
	    goto end;
	}
	// Skip CMF & FLG octets. zlib's inflate() function doesn't want them.
	la_inflate_result inflated = la_inflate(b64_decoded_msg->buf + 2, b64_decoded_msg->len - 2);

	if(inflated.success == false) {
	    la_debug_print(D_ERROR, "ZLIB decompressor failed\n");
	    msg->err = LA_OHMA_FAIL_DECOMPRESSION_FAILED;
	    LA_XFREE(inflated.buf);
	    goto end;
	}
#ifdef WITH_JANSSON
	json_error_t err;
	json_t *root = json_loadb((char const *)inflated.buf, inflated.buflen, 0, &err);
	if(!root) {
		la_debug_print(D_ERROR, "Failed to decode outer JSON string at position %d: %s\n",
				err.position, err.text);
		goto json_fail;
	}
	if(!json_is_object(root)) {
		la_debug_print(D_ERROR, "JSON root is not an object\n");
		json_decref(root);
		goto json_fail;
	}

	char *version = NULL, *convo_id = NULL, *message = NULL;
	int32_t msg_seq = 0, msg_total = 0;
	if(json_unpack_ex(root, &err, 0LU, "{s:s, s?:s, s:s, s?:i, s?:i}",
				"version", &version, "convo_id", &convo_id, "message", &message,
				"msg_seq", &msg_seq, "msg_total", &msg_total) < 0) {
		la_debug_print(D_INFO, "json_unpack_ex failed: %s\n", err.text);
		msg->err = LA_OHMA_JSON_BAD_STRUCTURE;
		json_decref(root);
		// FIXME: don't fail, just pretty-print the JSON
		goto json_fail;
	}
	msg->version = strdup(version);
	if(convo_id) {
		msg->convo_id = strdup(convo_id);
	}
	msg->msg_seq = msg_seq;
	if(msg_seq > 0) {
		// The message is fragmented. convo_id is required.
		if(convo_id == NULL) {
			la_debug_print(D_INFO, "JSON: msg_seq set, but convo_id missing\n");
			json_decref(root);
			goto json_fail;
		}
		// If this is the first fragment, then we also need msg_total.
		if(msg_seq == 1) {
			if(msg_total == 0) {
				la_debug_print(D_INFO, "JSON: msg_seq is 1, but msg_total is not present\n");
				json_decref(root);
				goto json_fail;
			}
		}
		// TODO: actually reassemble the message.
	} else {
		msg->reasm_status = LA_REASM_SKIPPED;
	}

	// Try to decode "message" field as JSON and pretty-print it if successful.
	// Otherwise just print it as is.
	if(msg->reasm_status == LA_REASM_SKIPPED) {
		json_t *inner_msg_root = json_loads(message, 0, &err);
		if(inner_msg_root) {
			char *pretty = json_dumps(inner_msg_root, JSON_INDENT(1) | JSON_REAL_PRECISION(6));
			if(pretty != NULL) {
				msg->payload = la_octet_string_new(pretty, strlen(pretty));
			} else {
				la_debug_print(D_INFO, "json_dumps() did not return any result\n");
				msg->payload = la_octet_string_new(strdup(message), strlen(message));
			}
			json_decref(inner_msg_root);
		} else {
			la_debug_print(D_ERROR, "Failed to decode inner JSON string at position %d: %s\n",
					err.position, err.text);
			msg->payload = la_octet_string_new(strdup(message), strlen(message));
		}
	}

	LA_XFREE(inflated.buf);
	json_decref(root);
	goto end;
#endif   // WITH_JANSSON
json_fail:
    // Failed to decode JSON string - either due to decoding error or
    // Jansson support disabled during build - just NULL-terminate
    // the unprocessed buffer and attach it as payload for printing.
	msg->err = LA_OHMA_JSON_DECODE_FAILED;
	inflated.buf[inflated.buflen] = '\0';
	msg->payload = la_octet_string_new(inflated.buf, inflated.buflen);
end:
	la_octet_string_destroy(b64_decoded_msg);
	return node;

#else   // !WITH_ZLIB
	LA_UNUSED(reg);
	LA_UNUSED(txt);
	LA_UNUSED(rtables);
	LA_UNUSED(tx_time);
	return NULL;
#endif  // WITH_ZLIB
}

static void la_ohma_msg_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_ohma_msg *msg = data;
	la_octet_string_destroy(msg->payload);
	LA_XFREE(msg->version);
	LA_XFREE(msg->convo_id);
	LA_XFREE(msg);
}

void la_ohma_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	la_ohma_msg const *msg = data;
	if(msg->err != LA_OHMA_SUCCESS) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable OHMA message\n");
		return;
	}
	LA_ISPRINTF(vstr, indent, "OHMA message:\n");
	indent++;
	LA_ISPRINTF(vstr, indent, "Version: %s\n", msg->version ? msg->version : "<unknown>");
	if(msg->convo_id) {
		LA_ISPRINTF(vstr, indent, "Msg ID: %s\n", msg->convo_id);
	}
	if(msg->msg_seq > 0) {      // Print this only for multipart messages
		LA_ISPRINTF(vstr, indent, "Msg seq: %d\n", msg->msg_seq);
	}
	LA_ISPRINTF(vstr, indent, "Reassembly: %s\n", la_reasm_status_name_get(msg->reasm_status));
	if(msg->payload != NULL) {
		if(is_printable(msg->payload->buf, msg->payload->len)) {
			LA_ISPRINTF(vstr, indent, "Message:\n");
			la_isprintf_multiline_text(vstr, indent + 1, (char *)msg->payload->buf);
		} else {
			LA_ISPRINTF(vstr, indent, "Data (%zu bytes):\n", msg->payload->len);
			char *hexdump = la_hexdump(msg->payload->buf, msg->payload->len);
			la_isprintf_multiline_text(vstr, indent + 1, hexdump);
			LA_XFREE(hexdump);
		}
	}
}

void la_ohma_format_json(la_vstring *vstr, void const *data);
la_proto_node *la_proto_tree_find_ohma(la_proto_node *root);

la_type_descriptor const la_DEF_ohma_msg = {
	.format_text = la_ohma_format_text,
	.format_json = NULL,
	.json_key = NULL,
	.destroy = &la_ohma_msg_destroy
};
