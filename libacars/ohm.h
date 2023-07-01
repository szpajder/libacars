/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_OHM_H
#define LA_OHM_H 1

#include <stdint.h>
#include <libacars/libacars.h>          // la_type_descriptor, la_proto_node
#include <libacars/util.h>              // la_octet_string
#include <libacars/reassembly.h>        // la_reasm_ctx

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LA_OHM_SUCCESS              = 0,
	LA_OHM_DECOMPRESSION_FAILED = 1,
} la_ohm_decoding_error_code;
#define LA_OHM_DECODING_ERROR_MAX 1

typedef struct {
	la_ohm_decoding_error_code err;     // message decoding error code
	la_octet_string *payload;           // message payload
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_ohm_msg;

// ohm.c
la_proto_node *la_ohm_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time);
void la_ohm_format_text(la_vstring *vstr, void const *data, int indent);
void la_ohm_format_json(la_vstring *vstr, void const *data);
la_proto_node *la_proto_tree_find_ohm(la_proto_node *root);

extern la_type_descriptor const la_DEF_ohm_msg;

#ifdef __cplusplus
}
#endif

#endif
