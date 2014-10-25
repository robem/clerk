#ifndef _CLERK_LIST_H
#define _CLERK_LIST_H

#include <stdlib.h>
#include <assert.h>

#include <clerk_log.h>

#define LIST_FOREACH(elem, list) \
  for (elem = list->first; \
       elem != NULL; \
       elem = elem->next)

typedef struct clrk_list_elem {
  void *data;
  struct clrk_list_elem *next;
  struct clrk_list_elem *prev;
} clrk_list_elem_t;

typedef struct clrk_list {
   clrk_list_elem_t *first;
   clrk_list_elem_t *last;
   unsigned num_of_elems;
} clrk_list_t;


void clrk_list_elem_free(clrk_list_elem_t *elem);

clrk_list_elem_t * clrk_list_add(clrk_list_t *list, void *data);
clrk_list_elem_t * clrk_list_elem_insert_after(clrk_list_t *list, clrk_list_elem_t *after, clrk_list_elem_t *move);
clrk_list_elem_t * clrk_list_elem_insert_before(clrk_list_t *list, clrk_list_elem_t *move, clrk_list_elem_t *before);
clrk_list_elem_t * clrk_list_elem_remove(clrk_list_t *list, clrk_list_elem_t *elem);
void*              clrk_list_elem_data(clrk_list_elem_t *elem);

#endif
