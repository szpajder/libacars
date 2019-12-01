/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>			// strlen()
#include <libacars/macros.h>		// la_assert()
#include <libacars/vstring.h>		// la_vstring
#include <libacars/util.h>		// LA_XCALLOC(), LA_XFREE()

static void la_json_trim_comma(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	size_t len = vstr->len;
	if(len > 0 && vstr->str[len-1] == ',') {
		vstr->str[len-1] = '\0';
		vstr->len--;
	}
}

static char *la_json_escapechars(char const * const str) {
	la_assert(str != NULL);
	bool needs_escaping = false;
	size_t orig_len = strlen(str);
	size_t new_len = orig_len;
	if(orig_len > 0) {
		for(size_t i = 0; i < orig_len; i++) {
			if(str[i] < ' ' || str[i] == '\"' || str[i] == '\\') {
				needs_escaping = true;
				new_len += 5;		// to fit the \uNNNN form
			}
		}
	}
	if(!needs_escaping) {
		return strdup(str);
	}
	char *ret = LA_XCALLOC(new_len + 1, sizeof(char));
	char *outptr = ret;
	for(size_t i = 0; i < orig_len; i++) {
		if(str[i] < ' ' || str[i] == '\"' || str[i] == '\\') {
			*outptr++ = '\\';
			switch(str[i]) {
				case '\b':
					*outptr++ = 'b';
					break;
				case '\t':
					*outptr++ = 't';
					break;
				case '\n':
					*outptr++ = 'n';
					break;
				case '\f':
					*outptr++ = 'f';
					break;
				case '\r':
					*outptr++ = 'r';
					break;
				case '\"':
					*outptr++ = '\"';
					break;
				case '\\':
					*outptr++ = '\\';
					break;
				default:
					sprintf(outptr, "u%04x", str[i]);
					outptr += 5;
			}
		} else {
			*outptr++ = str[i];
		}
	}
	return ret;
}

static inline void la_json_print_key(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	if(key != NULL && key[0] != '\0') {
// Warning: no character escaping is performed here. For libacars this is fine
// as all key names are static. Escaping them would add unnecessary overhead.
		la_vstring_append_sprintf(vstr, "\"%s\":", key);
	}
}

void la_json_append_bool(la_vstring * const vstr, char const * const key, bool const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s,", (val == true ? "true" : "false"));
}

void la_json_append_double(la_vstring * const vstr, char const * const key, double const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%f,", val);
}

void la_json_append_long(la_vstring * const vstr, char const * const key, long const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%ld,", val);
}

void la_json_append_string(la_vstring * const vstr, char const * const key, char const * const val) {
	la_assert(vstr != NULL);
	if(val == NULL) {
		return;
	}
	la_json_print_key(vstr, key);
	char *escaped = la_json_escapechars(val);
	la_vstring_append_sprintf(vstr, "\"%s\",", escaped);
	LA_XFREE(escaped);
}

void la_json_append_char(la_vstring * const vstr, char const * const key, char const val) {
	la_assert(vstr != NULL);
	char tmp[2] = { val, '\0' };
	la_json_append_string(vstr, key, tmp);
}

void la_json_object_start(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "{");
}

void la_json_object_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "},");
}

void la_json_array_start(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "[");
}

void la_json_array_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "],");
}

void la_json_append_octet_string(la_vstring * const vstr, char const * const key,
uint8_t const * const buf, size_t len) {
	la_assert(vstr != NULL);
	la_json_array_start(vstr, key);
	if(buf != NULL && len > 0) {
		for(size_t i = 0; i < len; i++) {
			la_json_append_long(vstr, NULL, buf[i]);
		}
	}
	la_json_array_end(vstr);
}

void la_json_start(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_object_start(vstr, NULL);
}

void la_json_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_object_end(vstr);
	la_json_trim_comma(vstr);
}
