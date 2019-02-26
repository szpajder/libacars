/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdio.h>		// fprintf
#include <stdint.h>
#include <stdlib.h>		// calloc, realloc, free
#include <string.h>		// strerror, strlen, strdup, strnlen, strspn
#include <time.h>		// struct tm
#include <errno.h>		// errno
#include <unistd.h>		// _exit
#include <libacars/macros.h>	// la_debug_print()
#include <libacars/util.h>

void *la_xcalloc(size_t nmemb, size_t size, const char *file, const int line, const char *func) {
	void *ptr = calloc(nmemb, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): calloc(%zu, %zu) failed: %s\n",
			file, line, func, nmemb, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_xrealloc(void *ptr, size_t size, const char *file, const int line, const char *func) {
	ptr = realloc(ptr, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): realloc(%zu) failed: %s\n",
			file, line, func, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_dict_search(const la_dict *list, int id) {
	if(list == NULL) return NULL;
	la_dict *ptr;
	for(ptr = (la_dict *)list; ; ptr++) {
		if(ptr->val == NULL) return NULL;
		if(ptr->id == id) return ptr->val;
	}
}

size_t la_slurp_hexstring(char* string, uint8_t **buf) {
	if(string == NULL)
		return 0;
	size_t slen = strlen(string);
	if(slen & 1)
		slen--;
	size_t dlen = slen / 2;
	if(dlen == 0)
		return 0;
	*buf = LA_XCALLOC(dlen, sizeof(uint8_t));

	for(size_t i = 0; i < slen; i++) {
		char c = string[i];
		int value = 0;
		if(c >= '0' && c <= '9') {
			value = (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			value = (10 + (c - 'A'));
		} else if (c >= 'a' && c <= 'f') {
			 value = (10 + (c - 'a'));
		} else {
			la_debug_print("stopped at invalid char %u at pos %zu\n", c, i);
			return i/2;
		}
		(*buf)[(i/2)] |= value << (((i + 1) % 2) * 4);
	}
	return dlen;
}

char *la_hexdump(uint8_t *data, size_t len) {
	static const char hex[] = "0123456789abcdef";
	if(data == NULL) return strdup("<undef>");
	if(len == 0) return strdup("<none>");

	size_t rows = len / 16;
	if((len & 0xf) != 0) {
		rows++;
	}
	size_t rowlen = 16 * 2 + 16;		// 32 hex digits + 16 spaces per row
	rowlen += 16;				// ASCII characters per row
	rowlen += 10;				// extra space for separators
	size_t alloc_size = rows * rowlen + 1;	// terminating NULL
	char *buf = LA_XCALLOC(alloc_size, sizeof(char));
	char *ptr = buf;
	size_t i = 0, j = 0;
	while(i < len) {
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				*ptr++ = hex[((data[j] >> 4) & 0xf)];
				*ptr++ = hex[data[j] & 0xf];
			} else {
				*ptr++ = ' ';
				*ptr++ = ' ';
			}
			*ptr++ = ' ';
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = ' ';
		*ptr++ = '|';
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				if(data[j] < 32 || data[j] > 126) {
					*ptr++ = '.';
				} else {
					*ptr++ = data[j];
				}
			} else {
				*ptr++ = ' ';
			}
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = '|';
		*ptr++ = '\n';
		i += 16;
	}
	return buf;
}

int la_strntouint16_t(char const *txt, int const charcnt) {
	if(	txt == NULL ||
		charcnt < 1 ||
		charcnt > 9 ||	// prevent overflowing int
		strnlen(txt, charcnt) < (size_t)charcnt)
	{
		return -1;
	}
	int ret = 0;
	int base = 1;
	int j;
	for(int i = 0; i < charcnt; i++) {
		j = charcnt - 1 - i;
		if(txt[j] < '0' || txt[j] > '9') {
			return -2;
		}
		ret += (txt[j] - '0') * base;
		base *= 10;
	}
	return ret;
}

// returns the length of the string ignoring the terminating
// newline characters
size_t chomped_strlen(char const *s) {
	char *p = strchr(s, '\0');
	size_t ret = p - s;
	while(--p >= s) {
		if(*p != '\n' && *p != '\r') {
			break;
		}
		ret--;
	}
	return ret;
}

// parse and perform basic sanitization of timestamp
// in YYMMDDHHMMSS format.
// Do not use strptime() - it's not available on WIN32
// Do not use sscanf() - it's too liberal on the input
// (we do not want whitespaces between fields, for example)
#define ATOI2(x,y) (10 * ((x) - '0') + ((y) - '0'))
char *la_simple_strptime(char const *s, struct tm *t) {
	if(strspn(s, "0123456789") < 12) {
		return NULL;
	}
	t->tm_year = ATOI2(s[0],  s[1]) + 100;
	t->tm_mon  = ATOI2(s[2],  s[3]) - 1;
	t->tm_mday = ATOI2(s[4],  s[5]);
	t->tm_hour = ATOI2(s[6],  s[7]);
	t->tm_min  = ATOI2(s[8],  s[9]);
	t->tm_sec  = ATOI2(s[10], s[11]);
	t->tm_isdst = -1;
	if(t->tm_mon > 11 || t->tm_mday > 31 || t->tm_hour > 23 || t->tm_min > 59 || t->tm_sec > 59) {
		return NULL;
	}
	return (char *)s + 12;
}
