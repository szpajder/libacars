/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_LIST_H
#define LA_LIST_H 1

#include <stddef.h>     // size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct la_list la_list;

struct la_list {
	void *data;
	la_list *next;
};

typedef int (la_list_compare_func)(void const *data1, void const *data2);

la_list *la_list_next(la_list const *l);
la_list *la_list_append(la_list *l, void *data);
la_list *la_list_prepend(la_list *l, void *data);
la_list *la_list_insert(la_list *l, void *data);
la_list *la_list_insert_sorted(la_list *list, void *data, la_list_compare_func *compare_nodes);
size_t la_list_length(la_list const *l);
void la_list_foreach(la_list *l, void (*cb)(), void *ctx);
void la_list_free(la_list *l);
void la_list_free_full(la_list *l, void (*node_free)());
void la_list_free_full_with_ctx(la_list *l, void (*node_free)(), void *ctx);

#ifdef __cplusplus
}
#endif

#endif // !LA_LIST_H
