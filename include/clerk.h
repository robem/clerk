#ifndef _CLERK_H
#define _CLERK_H

#include <stdbool.h>
#include <string.h>   // strcpy()
#include <stdlib.h>   // malloc()
#include <assert.h>

#include <termbox.h>

#include <clerk_draw.h>
#include <clerk_list.h>
#include <clerk_json.h>
#include <clerk_log.h>

#define CLRK_MIN_HEIGHT 10
#define CLRK_MIN_WIDTH  20

#define CLRK_CONFIG_FILE "clerk.json"
#define CLRK_CONFIG_X    "state"
#define CLRK_CONFIG_TEXT "text"

#define CLRK_NUM_PRJ  20
#define CLRK_NUM_TODO 30

// Strings
#define CLRK_PRJ_NAME_SIZE        30
#define CLRK_TODO_MESSAGE_SIZE   100
#define CLRK_INPUT_BUFFER_SIZE  1000

typedef enum todo_state {
  UNCHECKED,
  CHECKED,
  RUNNING,
  INFO
} todo_state_t;

typedef struct clrk_todo {
  char message[CLRK_TODO_MESSAGE_SIZE];
  todo_state_t state;
  bool visible;
} clrk_todo_t;

typedef struct clrk_project {
  char name[CLRK_PRJ_NAME_SIZE];
  clrk_list_t *todo_list;
  clrk_list_elem_t *current;
  bool visible;
} clrk_project_t;

typedef struct clrk_clerk {
  const char *json;
  clrk_list_t *project_list;
  clrk_list_elem_t *current;
} clrk_clerk_t;

/*
 * Initialize Clerk with default values and draws user interface.
 */
void clrk_init(const char *json);

/*
 * Create/add a new project
 */
clrk_project_t* clrk_project_add(const char *name);

/*
 * Set current project pointer
 */
clrk_list_elem_t* clrk_project_set_current(clrk_list_elem_t *elem);

/*
 * Remove current project
 */
void clrk_project_remove_current(void);

/*
 * Create/add a new todo
 */
clrk_todo_t* clrk_todo_add(const char *name);

/*
 * Set/Unset 'X' in current todo
 */
void clrk_todo_tick_off(void);

/*
 * Set/Unset '*' in current todo
 */
void clrk_todo_running(void);

/*
 * Set/Unset 'i' in current todo
 */
void clrk_todo_info(void);

/*
 * Start Clerk.
 * If this function returns then Clerk will end.
 */
void clrk_loop_normal(void);

#endif
