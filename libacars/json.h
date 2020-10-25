/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2020 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_JSON_H
#define LA_JSON_H 1

#include <stdbool.h>
#include <stdint.h>
#include <libacars/vstring.h>           // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

// json.c
void la_json_object_start(la_vstring * const vstr, char const * const key);
void la_json_object_end(la_vstring * const vstr);
void la_json_array_start(la_vstring * const vstr, char const * const key);
void la_json_array_end(la_vstring * const vstr);
void la_json_append_bool(la_vstring * const vstr, char const * const key, bool const val);
void la_json_append_double(la_vstring * const vstr, char const * const key, double const val);
void la_json_append_long(la_vstring * const vstr, char const * const key, long const val);
void la_json_append_char(la_vstring * const vstr, char const * const key, char const  val);
void la_json_append_string(la_vstring * const vstr, char const * const key, char const * const val);
void la_json_append_octet_string(la_vstring * const vstr, char const * const key,
		uint8_t const * const buf, size_t len);
void la_json_append_octet_string_as_string(la_vstring * const vstr, char const *key,
		uint8_t const * const buf, size_t len);
void la_json_start(la_vstring * const vstr);
void la_json_end(la_vstring * const vstr);

#ifdef __cplusplus
}
#endif

#endif // !LA_JSON_H
