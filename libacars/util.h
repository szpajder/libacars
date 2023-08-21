/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_UTIL_H
#define LA_UTIL_H 1
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>         // size_t
#include <stdlib.h>         // free()
#include <time.h>           // struct tm
#include "config.h"         // HAVE_STRSEP, WITH_LIBXML2 WITH_ZLIB
#ifdef WITH_LIBXML2
#include <libxml/tree.h>    // xmlBufferPtr
#endif

typedef struct {
	uint8_t *buf;
	size_t len;
} la_octet_string;

void *la_xcalloc(size_t nmemb, size_t size, char const *file, int line, char const *func);
void *la_xrealloc(void *ptr, size_t size, char const *file, int line, char const *func);

#define LA_XCALLOC(nmemb, size) la_xcalloc((nmemb), (size), __FILE__, __LINE__, __func__)
#define LA_XREALLOC(ptr, size) la_xrealloc((ptr), (size), __FILE__, __LINE__, __func__)
#define LA_XFREE(ptr) do { free(ptr); ptr = NULL; } while(0)

#ifdef HAVE_STRSEP
#include <string.h>
#define LA_STRSEP strsep
#else
char *la_strsep(char **stringp, char const *delim);
#define LA_STRSEP la_strsep
#endif

#ifndef HAVE_MEMMEM
void *memmem(void const *haystack, size_t haystack_len, void const *needle, size_t needle_len);
#endif

#define ATOI2(x,y) (10 * ((x) - '0') + ((y) - '0'))

size_t la_slurp_hexstring(char *string, uint8_t **buf);
char *la_hexdump(uint8_t *data, size_t len);
bool is_printable(uint8_t const *buf, uint32_t data_len);
int la_strntouint16_t(char const *txt, int charcnt);
size_t chomped_strlen(char const *s);
char *la_simple_strptime(char const *s, struct tm *t);
la_octet_string *la_octet_string_new(void *buf, size_t len);
void la_octet_string_destroy(void *ostring_ptr);
#ifdef WITH_LIBXML2
xmlBufferPtr la_prettify_xml(char const *buf);
#endif
uint32_t la_reverse(uint32_t v, int numbits);
la_octet_string *la_base64_decode(char const *input, size_t input_len);

#ifdef WITH_ZLIB
typedef struct {
	uint8_t *buf;
	size_t buflen;
	bool success;
} la_inflate_result;

la_inflate_result la_inflate(uint8_t const *buf, int in_len);
#endif
char *la_json_pretty_print(char const *json_string);


#endif // !LA_UTIL_H
