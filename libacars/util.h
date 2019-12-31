/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_UTIL_H
#define LA_UTIL_H 1
#include <stdint.h>
#include <stddef.h>		// size_t
#include <stdlib.h>		// free()
#include <time.h>		// struct tm
#include "config.h"		// HAVE_STRSEP

typedef struct {
	int id;
	void *val;
} la_dict;

typedef struct {
	uint8_t *buf;
	size_t len;
} la_octet_string;

void *la_xcalloc(size_t nmemb, size_t size, const char *file, const int line, const char *func);
void *la_xrealloc(void *ptr, size_t size, const char *file, const int line, const char *func);

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

void *la_dict_search(const la_dict *list, int id);
size_t la_slurp_hexstring(char *string, uint8_t **buf);
char *la_hexdump(uint8_t *data, size_t len);
int la_strntouint16_t(char const *txt, int const charcnt);
size_t chomped_strlen(char const *s);
char *la_simple_strptime(char const *s, struct tm *t);
la_octet_string *la_octet_string_new(void *buf, size_t len);
void la_octet_string_destroy(void *ostring_ptr);

#endif // !LA_UTIL_H
