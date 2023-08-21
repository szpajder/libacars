/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */

#include "config.h"                 // WITH_ZLIB, WITH_JANSSON
#define _GNU_SOURCE                 // for memmem()
#include <string.h>                 // strlen, memmem
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>               // struct timeval
#endif
#ifdef WITH_JANSSON
#include <jansson.h>
#endif
#include <libacars/libacars.h>      // la_proto_node, la_type_descriptor
#include <libacars/reassembly.h>
#include <libacars/util.h>          // la_base64_decode, la_json_pretty_print
#include <libacars/dict.h>          // la_dict, la_dict_search
#include <libacars/macros.h>        // la_debug_print()
#include <libacars/json.h>          // ja_json_*
#include <libacars/ohma.h>          // la_ohma_msg

/********************************************************************************
 * OHMA reassembly constants and callbacks
 ********************************************************************************/

#ifdef WITH_JANSSON
// Clean up stale reassembly entries every 20 OHMA messages.
#define LA_OHMA_REASM_TABLE_CLEANUP_INTERVAL 20

// OHMA reassembly timeout
static struct timeval const la_ohma_reasm_timeout = {
	.tv_sec = 1200,
	.tv_usec = 0
};

typedef struct {
	char *reg, *convo_id;
} la_ohma_key;

static uint32_t la_ohma_key_hash(void const *key) {
	la_ohma_key const *k = key;
	uint32_t h = la_hash_string(k->reg, LA_HASH_INIT);
	h = la_hash_string(k->convo_id, h);
	return h;
}

static bool la_ohma_key_compare(void const *key1, void const *key2) {
	la_ohma_key const *k1 = key1;
	la_ohma_key const *k2 = key2;
	return (!strcmp(k1->reg, k2->reg) &&
			!strcmp(k1->convo_id, k2->convo_id));
}

static void la_ohma_key_destroy(void *ptr) {
	if(ptr == NULL) {
		return;
	}
	la_ohma_key *key = ptr;
	la_debug_print(D_INFO, "DESTROY KEY %s %s\n", key->reg, key->convo_id);
	LA_XFREE(key->reg);
	LA_XFREE(key->convo_id);
	LA_XFREE(key);
}

static void *la_ohma_key_get(void const *msg) {
	la_assert(msg != NULL);
	la_ohma_msg const *amsg = msg;
	LA_NEW(la_ohma_key, key);
	key->reg = strdup(amsg->reg);
	key->convo_id = strdup(amsg->convo_id);
	la_debug_print(D_INFO, "ALLOC KEY %s %s\n", key->reg, key->convo_id);
	return (void *)key;
}

static void *la_ohma_tmp_key_get(void const *msg) {
	la_assert(msg != NULL);
	la_ohma_msg const *amsg = msg;
	LA_NEW(la_ohma_key, key);
	key->reg = (char *)amsg->reg;
	key->convo_id = (char *)amsg->convo_id;
	return (void *)key;
}

static la_reasm_table_funcs ohma_reasm_funcs = {
	.get_key = la_ohma_key_get,
	.get_tmp_key = la_ohma_tmp_key_get,
	.hash_key = la_ohma_key_hash,
	.compare_keys = la_ohma_key_compare,
	.destroy_key = la_ohma_key_destroy
};
#endif  // WITH_JANSSON

/********************************************************************************
 * OHMA parsing and formatting functions
 ********************************************************************************/

la_proto_node *la_ohma_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
#ifdef WITH_ZLIB
	if(txt == NULL) {
		return NULL;
	}
	size_t len = 0LU;
	char const *ptr = txt;
restart:
	len = strlen(ptr);
	// OHMA message recognition logic:
	// - downlinks, short form: starts with "OHMA" or "RYKO"
	// - downlinks, long form: additionally preceded with '/', 7-char ground address and '.' (eg. "/RTNBOCR.OHMA")
	// - uplinks: '/' + 2 characters + '.OHMA' or '.RYKO'
	if(len >= 13 && ptr[0] == '/' && ptr[8] == '.') {
	    ptr += 9; len -= 9;
	} else if(len >= 8 && ptr[0] == '/' && ptr[3] == '.') {
		ptr += 4; len -= 4;
	}
	if(strncmp(ptr, "OHMA", 4) == 0 || strncmp(ptr, "RYKO", 4) == 0) {
		ptr += 4; len -= 4;
	} else {
	    return NULL;
	}

	// This seems to be an OHMA message, but let's do an additional sanity
	// check.  If the message was reassembled from multiple ACARS blocks, it
	// sometimes happens that the contents of the second block is a duplicate
	// of the first block, however the sequence number has been incremented
	// correctly, an so the reassembly algorithm couldn't identify this as a
	// duplicate. This might be a bug in the ACARS sender software. So far I've
	// seen this only in uplink messages, Example:
	//
	// ACARS label H1, sublabel T1, Block Id: A, More 1: Contents: /O2.OHMAabcdefgh
	// ACARS label H1, sublabel T1, Block Id: B, More 1: Contents: /O2.OHMAabcdefgh
	// ACARS label H1, sublabel T1, Block Id: C, More 1: Contents: ijkl
	//
	// is reassembled to:
	//
	// /O2.OHMAabcdefgh/O2.OHMAabcdefghijkl
	//
	// while the correct result should be:
	//
	// /O2.OHMAabcdefghijkl
	//
	// Here we determine whether the initial part of the message (that is, the
	// prefix up to an including the "OHMA" string) occurs in the payload more
	// than once.  If it does, we skip it and restart the search for the OHMA
	// message.

	void *duplicate = memmem(ptr, len, txt, ptr - txt);
	if(duplicate != NULL) {
		la_debug_print(D_INFO, "Duplicate first fragment found; skipping it\n");
		ptr = duplicate;
		goto restart;
	}

	// Omit trailing CR+LFs, if present. They often appear in uplink messages
	// and they are not valid BASE64 characters.

	size_t new_len = len;
	while(new_len > 0 && (ptr[new_len-1] == '\r' || ptr[new_len-1] == '\n')) {
		new_len--;
	}

	la_debug_print(D_INFO, "Stripped %zu CR/LF characters before BASE64 decoding\n", len - new_len);
	la_octet_string *b64_decoded_msg = la_base64_decode(ptr, new_len);
	if(b64_decoded_msg == NULL) {
		la_debug_print(D_INFO, "Not an OHMA message (Failed to decode as BASE64)\n");
		// Fail silently without producing a node, since it's probably not an OHMA message
		return NULL;
	}

	LA_NEW(la_ohma_msg, msg);
	msg->err = LA_OHMA_SUCCESS;
	msg->reg = reg;
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
		msg->err = LA_OHMA_JSON_DECODE_FAILED;
		goto json_fail;
	}
	if(!json_is_object(root)) {
		la_debug_print(D_ERROR, "JSON root is not an object\n");
		msg->err = LA_OHMA_JSON_BAD_STRUCTURE;
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
		goto json_fail;
	}
	msg->version = strdup(version);
	if(convo_id) {
		msg->convo_id = strdup(convo_id);
	}
	char *sym_key = NULL, *iv = NULL, *signature = NULL;
	if(json_unpack_ex(root, &err, 0LU, "{s?:s, s?:s, s?:s}",
				"sym_key", &sym_key, "iv", &iv, "signature", &signature) < 0) {
		la_debug_print(D_INFO, "json_unpack_ex (2) failed: %s\n", err.text);
	} else {
		if(sym_key != NULL) {
			msg->sym_key = la_base64_decode(sym_key, strlen(sym_key));
		}
		if(iv != NULL) {
			msg->iv = la_base64_decode(iv, strlen(iv));
		}
		if(signature != NULL) {
			msg->signature = la_base64_decode(signature, strlen(signature));
		}
	}
	uint8_t *reassembled_message = NULL;
	if(msg_seq > 0) {
		msg->msg_seq = msg_seq;
		// The message is fragmented. convo_id is required.
		if(convo_id == NULL) {
			la_debug_print(D_INFO, "JSON: msg_seq set, but convo_id missing\n");
			msg->err = LA_OHMA_JSON_BAD_STRUCTURE;
			json_decref(root);
			goto json_fail;
		}
		// If this is the first fragment, then we also need msg_total.
		if(msg_seq == 1 && msg_total == 0) {
			la_debug_print(D_INFO, "JSON: msg_seq is 1, but msg_total is not present\n");
			msg->err = LA_OHMA_JSON_BAD_STRUCTURE;
			json_decref(root);
			goto json_fail;
		}
		msg->msg_total = msg_total;

		la_reasm_table *ohma_rtable = NULL;
		if(rtables != NULL) {       // reassembly engine is enabled
			ohma_rtable = la_reasm_table_lookup(rtables, &la_DEF_ohma_msg);
			if(ohma_rtable == NULL) {
				ohma_rtable = la_reasm_table_new(rtables, &la_DEF_ohma_msg,
						ohma_reasm_funcs, LA_OHMA_REASM_TABLE_CLEANUP_INTERVAL);
			}
			msg->reasm_status = la_reasm_fragment_add(ohma_rtable,
					&(la_reasm_fragment_info){
					.msg_info = msg,
					.msg_data = (uint8_t *)message,
					.msg_data_len = strlen(message),
					.total_pdu_len = 0,        // not used here
					.rx_time = rx_time,
					.reasm_timeout = la_ohma_reasm_timeout,
					.seq_num = msg->msg_seq,
					.seq_num_first = 1,
					.seq_num_wrap = SEQ_WRAP_NONE,
					.is_final_fragment = false,
					.total_fragment_cnt = msg_total,
					.flags = LA_ALLOW_OUT_OF_ORDER_DELIVERY
					});
			if(msg->reasm_status == LA_REASM_COMPLETE) {
				la_reasm_payload_get(ohma_rtable, msg, &reassembled_message);
			}
		}
	} else {
		msg->reasm_status = LA_REASM_SKIPPED;
	}

	if(reassembled_message != NULL) {
		msg->payload = la_octet_string_new(reassembled_message,
				strlen((char *)reassembled_message));
	} else {
		msg->payload = la_octet_string_new(strdup(message), strlen(message));
	}

	LA_XFREE(inflated.buf);
	json_decref(root);
	goto end;
json_fail:
#else   // !WITH_JANSSON
	LA_UNUSED(rtables);
	LA_UNUSED(rx_time);
#endif   // WITH_JANSSON
    // Failed to decode JSON string - either due to decoding error or
    // Jansson support disabled during build - just NULL-terminate
    // the unprocessed buffer and attach it as payload for printing.
	inflated.buf[inflated.buflen] = '\0';
	msg->payload = la_octet_string_new(inflated.buf, inflated.buflen);
end:
	la_octet_string_destroy(b64_decoded_msg);
	return node;

#else   // !WITH_ZLIB
	LA_UNUSED(reg);
	LA_UNUSED(txt);
	return NULL;
#endif  // WITH_ZLIB
}

static void la_ohma_msg_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_ohma_msg *msg = data;
	la_octet_string_destroy(msg->payload);
	la_octet_string_destroy(msg->sym_key);
	la_octet_string_destroy(msg->iv);
	la_octet_string_destroy(msg->signature);
	LA_XFREE(msg->version);
	LA_XFREE(msg->convo_id);
	LA_XFREE(msg);
}

static void la_print_hexdump(la_vstring *vstr, int indent, la_octet_string *ostring) {
	la_assert(vstr);
	la_assert(ostring);
	char *hexdump = la_hexdump(ostring->buf, ostring->len);
	la_isprintf_multiline_text(vstr, indent, hexdump);
	LA_XFREE(hexdump);
}

void la_ohma_format_text(la_vstring *vstr, void const *data, int indent) {
	static la_dict const la_ohma_decoding_error_descriptions[] = {
		{ .id = LA_OHMA_SUCCESS, .val = "Success" },
		{ .id = LA_OHMA_FAIL_MSG_TOO_SHORT, .val = "Message too short" },
		{ .id = LA_OHMA_FAIL_UNKNOWN_COMPRESSION, .val = "Unknown compression algorithm" },
		{ .id = LA_OHMA_FAIL_DECOMPRESSION_FAILED, .val = "Decompression failed" },
		{ .id = LA_OHMA_JSON_DECODE_FAILED, .val = "Failed to decode message as JSON" },
		{ .id = LA_OHMA_JSON_BAD_STRUCTURE, .val = "Unexpected JSON structure" }
	};
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	la_ohma_msg const *msg = data;
	LA_ISPRINTF(vstr, indent, "OHMA message:\n");
	indent++;
	if(msg->err != LA_OHMA_SUCCESS) {
		char const *err_string = la_dict_search(la_ohma_decoding_error_descriptions, msg->err);
		la_assert(err_string);
		LA_ISPRINTF(vstr, indent, "-- %s\n", err_string);
	} else {
		if(msg->version) {
			LA_ISPRINTF(vstr, indent, "Version: %s\n", msg->version);
		}
		if(msg->convo_id) {
			LA_ISPRINTF(vstr, indent, "Msg ID: %s\n", msg->convo_id);
		}
		if(msg->msg_seq > 0) {      // Print this only for multipart messages
			LA_ISPRINTF(vstr, indent, "Msg seq: %d\n", msg->msg_seq);
		}
		if(msg->msg_total > 0) {
			LA_ISPRINTF(vstr, indent, "Msg total: %d\n", msg->msg_total);
		}
		LA_ISPRINTF(vstr, indent, "Reassembly: %s\n", la_reasm_status_name_get(msg->reasm_status));
		if(msg->sym_key) {
			LA_ISPRINTF(vstr, indent, "Sym key:\n");
			la_print_hexdump(vstr, indent + 1, msg->sym_key);
		}
		if(msg->iv) {
			LA_ISPRINTF(vstr, indent, "IV:\n");
			la_print_hexdump(vstr, indent + 1, msg->iv);
		}
		if(msg->signature) {
			LA_ISPRINTF(vstr, indent, "Signature:\n");
			la_print_hexdump(vstr, indent + 1, msg->signature);
		}
	}
	if(msg->payload != NULL) {
		if(is_printable(msg->payload->buf, msg->payload->len)) {
			// msg->payload is guaranteed to be NULL-terminated, so a cast to char * is safe.
			char *pretty = la_json_pretty_print((char *)msg->payload->buf);
			if(pretty != NULL) {
				LA_ISPRINTF(vstr, indent, "Message (reformatted):\n");
				la_isprintf_multiline_text(vstr, indent + 1, pretty);
				LA_XFREE(pretty);
			} else {
				// Result might be NULL either due to pretty-printing being
				// disabled in the config or the payload not being JSON.
				// In either case, print the message without reformatting.
				LA_ISPRINTF(vstr, indent, "Message:\n");
				la_isprintf_multiline_text(vstr, indent + 1, (char *)msg->payload->buf);
			}
		} else {
			LA_ISPRINTF(vstr, indent, "Data (%zu bytes):\n", msg->payload->len);
			la_print_hexdump(vstr, indent + 1, msg->payload);
		}
	}
}

void la_ohma_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_ohma_msg const *msg = data;
	la_json_append_int64(vstr, "err", msg->err);
	if(msg->err == LA_OHMA_SUCCESS) {
		if(msg->version) {
			la_json_append_string(vstr, "version", msg->version);
		}
		if(msg->convo_id) {
			la_json_append_string(vstr, "msg_id", msg->convo_id);
		}
		if(msg->msg_seq > 0) {
			la_json_append_int64(vstr, "msg_seq", msg->msg_seq);
		}
		if(msg->msg_total > 0) {
			la_json_append_int64(vstr, "msg_total", msg->msg_total);
		}
		la_json_append_string(vstr, "reasm_status", la_reasm_status_name_get(msg->reasm_status));
		if(msg->sym_key != NULL) {
			la_json_append_octet_string(vstr, "sym_key", msg->sym_key->buf, msg->sym_key->len);
		}
		if(msg->iv != NULL) {
			la_json_append_octet_string(vstr, "iv", msg->iv->buf, msg->iv->len);
		}
		if(msg->signature != NULL) {
			la_json_append_octet_string(vstr, "signature", msg->signature->buf, msg->signature->len);
		}
		if(msg->payload != NULL) {
			if(is_printable(msg->payload->buf, msg->payload->len)) {
				la_json_append_string(vstr, "text", (char *)msg->payload->buf);
			} else {
				la_json_append_octet_string(vstr, "octet_string",
						msg->payload->buf, msg->payload->len);
			}
		}
	}
}

la_proto_node *la_proto_tree_find_ohma(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_ohma_msg);
}

la_type_descriptor const la_DEF_ohma_msg = {
	.format_text = la_ohma_format_text,
	.format_json = la_ohma_format_json,
	.json_key = "ohma",
	.destroy = &la_ohma_msg_destroy
};
