#include <clerk_list.h>

static clrk_list_elem_t * clrk_list_new(void* data)
{
  HERE();
  clrk_list_elem_t *new = malloc(sizeof(clrk_list_elem_t));
  assert(new);

  LOG("new elem "PTR" and new data "PTR, new, data);
  new->data = data;
  new->next = NULL;
  new->prev = NULL;

  return new;
}

void clrk_list_elem_free(clrk_list_elem_t *elem)
{
  HERE();
  free(elem);
}

clrk_list_elem_t * clrk_list_add(clrk_list_t *list, void *data)
{
  HERE();
  clrk_list_elem_t *elem;

  assert(list);

  elem = clrk_list_new(data);

  if (list->last) {
     LOG("Add elem to list");
     list->last->next = elem;
     elem->prev       = list->last;
     list->last       = elem;
  } else {
     LOG("Add first elem to list");
     list->first = elem;
     list->last  = elem;
  }

  return elem;
}

clrk_list_elem_t * clrk_list_elem_insert_after(clrk_list_t *list, clrk_list_elem_t *after, clrk_list_elem_t *move)
{
  HERE();
  assert(move);
  assert(after);

  if (after->next) {
    after->next->prev = move;
  } else {
    list->last = move;
  }
  move->next = after->next;
  move->prev = after;
  after->next = move;

  return move;
}

clrk_list_elem_t * clrk_list_elem_insert_before(clrk_list_t *list, clrk_list_elem_t *move, clrk_list_elem_t *before)
{
  HERE();
  assert(move);
  assert(before);

  if (before->prev) {
    before->prev->next = move;
  } else {
    list->first = move;
  }
  move->prev = before->prev;
  move->next = before;
  before->prev = move;

  return move;
}

clrk_list_elem_t* clrk_list_elem_remove(clrk_list_t *list, clrk_list_elem_t *elem)
{
  HERE();
  assert(list);
  assert(elem);

  if (elem->prev) {
    elem->prev->next = elem->next;
  }

  if (elem->next) {
    elem->next->prev = elem->prev;
  }

  if (elem == list->first) {
     list->first = elem->next;
  }

  if (elem == list->last) {
     list->last = elem->prev;
  }

  list->num_of_elems--;
  assert(list->num_of_elems >= 0);

  return elem;
}

void* clrk_list_elem_data(clrk_list_elem_t *elem)
{
  HERE();
  return elem ? elem->data : NULL;
}

