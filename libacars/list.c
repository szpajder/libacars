/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <libacars/macros.h>    // la_assert
#include <libacars/list.h>      // la_list
#include <libacars/util.h>      // LA_XCALLOC, LA_XFREE

la_list *la_list_next(la_list const *l) {
	if(l == NULL) {
		return NULL;
	}
	return l->next;
}

la_list *la_list_append(la_list *l, void *data) {
	LA_NEW(la_list, new);
	new->data = data;
	if(l == NULL) {
		return new;
	} else {
		la_list *ptr;
		for(ptr = l; ptr->next != NULL; ptr = la_list_next(ptr))
			;
		ptr->next = new;
		return l;
	}
}

// Inserts a new element at position l->next.
// Returns a pointer to the newly inserted element
// (which might be a new list if l == NULL).
la_list *la_list_insert(la_list *l, void *data) {
	if(l == NULL) {
		return la_list_append(l, data);
	}
	la_list *next = l->next;
	l->next = NULL;
	l = la_list_append(l, data);
	l->next->next = next;
	return l->next;
}

// Prepends a new element to the list l.
// Returns a pointer to the new list head.
la_list *la_list_prepend(la_list *l, void *data) {
	LA_NEW(la_list, new);
	new->data = data;
	new->next = l;
	return new;
}

// Inserts a new element into the list, preserving sort order of the elements.
// The order is determined by the compare_nodes callback, which shall return a
// value <0, 0 or >0, when the first argument is less, equal or greater than
// the second one, respectively.  If list is NULL, a new list is allocated and
// the element is appended to it.
la_list *la_list_insert_sorted(la_list *list, void *data, la_list_compare_func *compare_nodes) {
	if(list == NULL) {
		return la_list_append(list, data);
	}
	if(compare_nodes(list->data, data) > 0) {
		return la_list_prepend(list, data);
	}
	for(la_list *l = list; l->next != NULL; l = la_list_next(l)) {
		if(compare_nodes(l->next->data, data) > 0) {
			(void)la_list_insert(l, data);
			return list;
		}
	}
	return la_list_append(list, data);
}

size_t la_list_length(la_list const *l) {
	size_t len = 0;
	for(; l != NULL; l = la_list_next(l), len++)
		;
	return len;
}

void la_list_foreach(la_list *l, void (*cb)(), void *ctx) {
	la_assert(cb != NULL);
	for(; l != NULL; l = la_list_next(l)) {
		cb(l->data, ctx);
	}
}

void la_list_free_full_with_ctx(la_list *l, void (*node_free)(), void *ctx) {
	if(l == NULL) {
		return;
	}
	la_list_free_full_with_ctx(l->next, node_free, ctx);
	l->next = NULL;
	if(node_free != NULL) {
		node_free(l->data, ctx);
	} else {
		LA_XFREE(l->data);
	}
	LA_XFREE(l);
}

void la_list_free_full(la_list *l, void (*node_free)()) {
	if(l == NULL) {
		return;
	}
	la_list_free_full(l->next, node_free);
	l->next = NULL;
	if(node_free != NULL) {
		node_free(l->data);
	} else {
		LA_XFREE(l->data);
	}
	LA_XFREE(l);
}

void la_list_free(la_list *l) {
	la_list_free_full(l, NULL);
}
