#ifndef _CLERK_LIST_H
#define _CLERK_LIST_H

#include <stdlib.h>
#include <assert.h>

#include <clerk_log.h>

#define LIST_FOREACH(e, list) \
  for (elem_ptr = list->first, \
      e = elem_ptr?clrk_list_elem_data(elem_ptr):NULL;\
      elem_ptr; \
      elem_ptr = elem_ptr->next, \
      e = elem_ptr?clrk_list_elem_data(elem_ptr):NULL)

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

static clrk_list_elem_t *elem_ptr;

void clrk_list_elem_free(clrk_list_elem_t *elem);

clrk_list_elem_t*  clrk_list_add(clrk_list_t *list, void *data);
clrk_list_elem_t*  clrk_list_elem_remove(clrk_list_t *list, clrk_list_elem_t *elem);
void*              clrk_list_elem_data(clrk_list_elem_t *elem);

#endif
