/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LA_LIST_H
#define LA_LIST_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct la_list la_list;

struct la_list {
	void *data;
	la_list *next;
};

la_list *la_list_next(la_list const * const l);
la_list *la_list_append(la_list *l, void *data);
size_t la_list_length(la_list const *l);
void la_list_foreach(la_list *l, void (*cb)(), void *ctx);
void la_list_free(la_list *l);
void la_list_free_full(la_list *l, void (*node_free)());

#ifdef __cplusplus
}
#endif

#endif // !LA_LIST_H
