/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stddef.h>
#include <libacars/dict.h>

void *la_dict_search(la_dict const *list, int id) {
	if(list == NULL) return NULL;
	la_dict *ptr;
	for(ptr = (la_dict *)list; ; ptr++) {
		if(ptr->val == NULL) return NULL;
		if(ptr->id == id) return ptr->val;
	}
}
