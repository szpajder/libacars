/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_JSON_H
#define LA_JSON_H 1

#include <stdbool.h>
#include <libacars/vstring.h>		// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

// json.c
void la_json_object_start(la_vstring * const vstr, char const * const key);
void la_json_object_end(la_vstring * const vstr);
void la_json_append_bool(la_vstring * const vstr, char const * const key, bool const val);
void la_json_append_string(la_vstring * const vstr, char const * const key, char const * const val);

#ifdef __cplusplus
}
#endif

#endif // !LA_JSON_H
