#include <clerk_list.h>

static clrk_list_t* clrk_list_new(void* data)
{
  HERE();
  clrk_list_t *new;
  new = (clrk_list_t*)malloc(sizeof(clrk_list_t));
  assert(new);

  new->data = data;
  new->next = NULL;
  new->prev = NULL;

  return new;
}

void clrk_list_free(clrk_list_t *list)
{
  HERE();
  free(list);
}

clrk_list_t* clrk_list_add(clrk_list_t **list, void *data)
{
  HERE();
  clrk_list_t *p, *elem;

  if (list && *list == NULL) {
    *list = clrk_list_new(data);
    return *list;
  }

  p = *list;
  while (p->next)
    p = p->next;

  elem = clrk_list_new(data);

  p->next = elem;
  elem->prev = p;

  return elem;
}

clrk_list_t* clrk_list_remove(clrk_list_t *elem)
{
  HERE();

  if (elem && elem->prev) {
    elem->prev->next = elem->next;
  }
  if (elem && elem->next) {
    elem->next->prev = elem->prev;
  }

  return elem;
}

void* clrk_list_data(clrk_list_t *list)
{
  HERE();
  return list ? list->data : NULL;
}

clrk_list_t* clrk_list_next(clrk_list_t* list)
{
  HERE();
  return list ? list->next : NULL;
}

clrk_list_t* clrk_list_prev(clrk_list_t* list)
{
  HERE();
  return list ? list->prev : NULL;
}

