#ifndef _CLERK_LIST_H
#define _CLERK_LIST_H

#include <stdlib.h>
#include <assert.h>

#include <clerk_log.h>

#define FOR_EACH(e, list) \
  clrk_list_t *clrk_list_ptr; \
  for (clrk_list_ptr = list, \
      e = clrk_list_ptr?clrk_list_ptr->data:NULL;\
      clrk_list_ptr; \
      clrk_list_ptr = clrk_list_ptr->next, \
      e = clrk_list_ptr?clrk_list_ptr->data:NULL)

typedef struct clrk_list {
  void *data;
  struct clrk_list *next;
  struct clrk_list *prev;
} clrk_list_t;

void clrk_list_free(clrk_list_t *list);

clrk_list_t*  clrk_list_add(clrk_list_t **list, void *data);
clrk_list_t*  clrk_list_insert(clrk_list_t* list, clrk_list_t* elem);
clrk_list_t*  clrk_list_remove(clrk_list_t *elem);
void*         clrk_list_data(clrk_list_t *list);
clrk_list_t*  clrk_list_next(clrk_list_t *list);
clrk_list_t*  clrk_list_prev(clrk_list_t *list);

#endif
