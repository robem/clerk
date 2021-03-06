#include <clerk.h>

clrk_clerk_t clerk;

static void clrk_input(char *text, char *buffer)
{
  HERE();
  unsigned i, cx, cy;
  struct tb_event event;
  bool space = false;

  memset((void*)buffer, '\0', CLRK_INPUT_BUFFER_SIZE);

  cy = tb_height() - 1;
  cx = 2;

  clrk_draw_show_input_line();
  tb_present();

  /* Draw given text and set cursor at the end */
  if (text) {
    strncpy(buffer, text, CLRK_INPUT_BUFFER_SIZE);
    clrk_draw_text(cx, cy, text, 15, CLRK_COLOR_INPUT_BG);
    cx += strlen(text);
    tb_set_cursor(cx, cy);
    tb_present();
  }

  /* Start input loop */
  while (tb_poll_event(&event)) {
    i = cx - 2;
    if (event.type == TB_EVENT_KEY) {
      space = (event.key == TB_KEY_SPACE);

      if (event.key == TB_KEY_ESC) {
        break;
      } else if (event.key == TB_KEY_ENTER) {
        clrk_draw_remove_input_line();
        LOG("END; return buffer \"%s\" @ %p", buffer, buffer);
        return;
      } else if (event.key == TB_KEY_BACKSPACE || event.key == TB_KEY_BACKSPACE2) {
        if (i > 0) {
          size_t tail = strlen(&buffer[i]);
          if (tail) {
            LOG("copy tail [%zu]", tail);
            /* Shift tail by 1 to the left */
            char *helper = strdup(&buffer[i]);
            memcpy(&buffer[i-1], helper, strlen(helper));
            free(helper);
            /* Paint over last character */
            buffer[i+tail-1] = ' ';
            clrk_draw_text(cx - 1, cy, &buffer[i-1], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
            /* Clear last character */
            buffer[i+tail-1] = '\0';
          } else {
            LOG("remove last char");
            buffer[i-1] = '\0';
            tb_change_cell(cx - 1, cy, '\0', CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
          }
          tb_set_cursor(--cx, cy);
        }
      } else if ((event.ch > 31 && event.ch < 127) || space) {
        LOG("input [%d] key %c", i, event.ch);
        if (i < CLRK_INPUT_BUFFER_SIZE) {
          if (buffer[i] == '\0') {
            // Append character
            tb_change_cell(cx, tb_height() - 1, event.ch, CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
            buffer[i] = space ? ' ' : event.ch;
          } else {
            // Add character in between
            char *helper = strdup(&buffer[i]);
            memcpy(&buffer[i+1], helper, strlen(helper));
            free(helper);
            buffer[i] = space ? ' ' : event.ch;
            clrk_draw_text(cx, cy, &buffer[i], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
          }
          tb_set_cursor(++cx, cy);
        }
      } else if (event.key == TB_KEY_ARROW_LEFT) {
        if (i > 0) {
          tb_set_cursor(--cx, cy);
        }
      } else if (event.key == TB_KEY_ARROW_RIGHT) {
        if (i < CLRK_INPUT_BUFFER_SIZE && buffer[i] != '\0') {
          tb_set_cursor(++cx, cy);
        }
      }
      LOG("key %d", event.key);
    } else if (event.type == TB_EVENT_RESIZE) {
    }
    tb_present();
  }

  clrk_draw_remove_input_line();
  return;
}

static clrk_project_t * clrk_project_new(const char *name)
{
  HERE();
  clrk_project_t *project = malloc(sizeof(clrk_project_t));
  assert(project);

  memset((void*)project->name, 0, CLRK_PRJ_NAME_SIZE);
  if (name && *name != '\0') {
    strncpy(project->name, name, CLRK_PRJ_NAME_SIZE);
  } else {
    /* Set default name if name is empty string or nullptr */
    strncpy(project->name, "NONAME", 7);
  }
  memset((void*)name, 0, CLRK_PRJ_NAME_SIZE);

  project->current = NULL;
  project->todo_list = malloc(sizeof(clrk_list_t));
  assert(project->todo_list);
  project->todo_list->first        = NULL;
  project->todo_list->last         = NULL;
  project->todo_list->num_of_elems = 0;

  project->visible = false;

  LOG("new project: %s "PTR, project->name, project);
  LOG("END");
  return project;
}

clrk_project_t * clrk_project_add(const char *name)
{
  HERE();
  clrk_list_elem_t *elem;
  clrk_project_t *project;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);

  if (name == NULL) {
    /* Create projects interactively */
    clrk_input(NULL, buffer);
  } else {
    /* Create projects while parsing json file */
    strncpy(buffer, name, CLRK_INPUT_BUFFER_SIZE);
  }

  project = clrk_project_new(buffer);

  free(buffer);

  elem = clrk_list_add(clerk.project_list, project);
  clerk.current = elem;

  clrk_draw();

  LOG("END");
  return project;
}

clrk_list_elem_t * clrk_project_set_current(clrk_list_elem_t *elem)
{
  HERE();

  if (elem) {
    clerk.current = elem;
    clrk_draw();
  }

  LOG("END");
  return elem;
}

static void clrk_project_move_right(void)
{
   HERE();

   if (clerk.current && clerk.current->next) {
      clrk_list_elem_t *helper = clerk.current->next;
      clrk_list_elem_remove(clerk.project_list, clerk.current);
      clrk_list_elem_insert_after(clerk.project_list, helper, clerk.current);

      clrk_draw_project_line();
   }

   LOG("END");
}

static void clrk_project_move_left(void)
{
   HERE();

   if (clerk.current && clerk.current->prev) {
      clrk_list_elem_t *helper = clerk.current->prev;
      clrk_list_elem_remove(clerk.project_list, clerk.current);
      clrk_list_elem_insert_before(clerk.project_list, clerk.current, helper);

      clrk_draw_project_line();
   }

   LOG("END");
}

static void clrk_project_edit_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);

  if (buffer == NULL) {
    LOG("Couldn't allocate input buffer");
    return;
  }

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    clrk_input(project->name, buffer);
    if (buffer == '\0') {
      /* No empty string permitted */
      return;
    }
    strncpy(project->name, buffer, CLRK_PRJ_NAME_SIZE);
    clrk_draw_project_line();
  }

  free(buffer);
}

void clrk_project_remove_current(void)
{
  HERE();
  int i;
  clrk_project_t *project;

  if (clerk.project_list == NULL || clerk.current == NULL) {
    LOG("No active project found.");
    return;
  }

  clrk_list_elem_t *e = clerk.current;
  if (e == clerk.project_list->last && e != clerk.project_list->first) {
    /* Move visible 'window' by 1 to the left if elem was last */
    LIST_FOREACH(e, clerk.project_list) {
       project = clrk_list_elem_data(e);
       if (project->visible) {
          break;
       }
    }
    if (e != clerk.project_list->first) {
       project = clrk_list_elem_data(e->prev);
       project->visible = true;
    }
  } else if (e != clerk.project_list->first) {
    /* Move visible 'window' by 1 to the rigth otherwise */
    do {
       e = e->next;
       project = clrk_list_elem_data(e);
    } while (project->visible && e->next);
    project->visible = true;
  }

  /* Remove from project list */
  clrk_list_elem_remove(clerk.project_list, clerk.current);

  /* Remove the whole project's todo list */
  project = clrk_list_elem_data(clerk.current);
  clrk_list_elem_t *helper_elem;
  clrk_list_elem_t *todo_elem = project->todo_list->first;
  while (todo_elem) {
    free(clrk_list_elem_data(todo_elem));

    helper_elem = todo_elem->next;
    clrk_list_elem_free(todo_elem);
    todo_elem = helper_elem;
  }
  free(project->todo_list);

  /* Destroy element */
  helper_elem = clerk.current->prev?clerk.current->prev:clerk.current->next;
  clrk_list_elem_free(clerk.current);

  /* Set new current project element */
  clerk.current = helper_elem;

  /* Force re-calculation of visible projects */
  LOG("END");
}

static clrk_todo_t * clrk_todo_new(const char *text)
{
  HERE();
  clrk_todo_t *todo = malloc(sizeof(clrk_todo_t));
  assert(todo);

  strncpy(todo->message, text, CLRK_TODO_MESSAGE_SIZE);
  todo->state = UNCHECKED;
  todo->visible = false;

  LOG("Add new todo: %s", text);

  LOG("END");
  return todo;
}

clrk_todo_t * clrk_todo_add(const char *text)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  clrk_list_elem_t *elem;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);

  if (buffer == NULL) {
    LOG("Couldn't allocate input buffer.");
    return NULL;
  }

  if (clerk.project_list == NULL) {
    return NULL;
  }

  project = clrk_list_elem_data(clerk.current);

  if (text == NULL) {
    clrk_input(NULL, buffer);
  } else {
    strncpy(buffer, text, CLRK_INPUT_BUFFER_SIZE);
  }

  todo = clrk_todo_new(buffer);
  assert(todo);

  free(buffer);

  elem = clrk_list_add(project->todo_list, todo);
  assert(elem);

  project->current = elem;
  clrk_draw_todos();

  LOG("END");
  return todo;
}

static void clrk_todo_edit_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);

  if (buffer == NULL) {
    LOG("Couldn't allocate input buffer.");
    return;
  }

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    if (project->current) {
      todo = clrk_list_elem_data(project->current);

      clrk_input(todo->message, buffer);
      if (buffer == '\0') {
        /* Deleting the whole todo text is not permitted. Use 'T' instead. */
        goto end;
      }
      strncpy(todo->message, buffer, CLRK_TODO_MESSAGE_SIZE);
      clrk_draw_todos();
    }
  }

end:
  free(buffer);
  return;
}

static void clrk_todo_remove_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    if (project->current) {
      clrk_list_elem_t *e = project->current;
      if (e == project->todo_list->last && e != project->todo_list->first) {
         /* Move visible 'window' by 1 to the left if elem was last */
         LIST_FOREACH(e, project->todo_list) {
            todo = clrk_list_elem_data(e);
            if (todo->visible) {
               break;
            }
         }
         if (e != project->todo_list->first) {
            todo = clrk_list_elem_data(e->prev);
            todo->visible = true;
         }
      } else if (e != project->todo_list->first) {
         /* Move visible 'window' by 1 to the rigth otherwise */
         do {
            e = e->next;
            todo = clrk_list_elem_data(e);
         } while (todo->visible && e->next);
         todo->visible = true;
      }

      clrk_list_elem_t *helper = project->current->prev?project->current->prev:project->current->next;
      clrk_list_elem_remove(project->todo_list, project->current);
      clrk_list_elem_free(project->current);

      if (project->current == project->todo_list->first) {
        project->todo_list->first = NULL;
        if (project->current->next) {
          project->todo_list->first = project->current->next;
        }
      }
      project->current = helper;

      LOG("END");
      clrk_draw_todos();
    }
  }
}

static clrk_list_elem_t * clrk_todo_select_next(void) {
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    if (project && project->current && project->current->next) {
      project->current = project->current->next;
      clrk_draw_todos();
    }
  }

  LOG("END");
  return project->current;
}

static clrk_list_elem_t * clrk_todo_select_prev(void)
{
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    if (project && project->current && project->current->prev) {
      project->current = project->current->prev;
      clrk_draw_todos();
    }
  }

  return project->current;
}

static clrk_list_elem_t * clrk_todo_move_up(void)
{
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    if (project->current->prev) {
      clrk_list_elem_t *helper = project->current->prev;
      clrk_list_elem_remove(project->todo_list, project->current);
      clrk_list_elem_insert_before(project->todo_list, project->current, helper);

      clrk_draw_todos();
    }
  }

  return project->current;
}

static clrk_list_elem_t * clrk_todo_move_down(void)
{
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    if (project->current->next) {
      clrk_list_elem_t *helper = project->current->next;
      clrk_list_elem_remove(project->todo_list, project->current);
      clrk_list_elem_insert_after(project->todo_list, helper, project->current);

      clrk_draw_todos();
    }
  }

  return project->current;
}

void clrk_todo_tick_off(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    if (project->current) {
      todo = clrk_list_elem_data(project->current);
      todo->state = (todo->state == CHECKED)?UNCHECKED:CHECKED;
      clrk_draw_todos();
    }
  }

  LOG("END");
}

static void clrk_todo_select_last(void)
{
  HERE();
  clrk_project_t *project;
  clrk_list_elem_t *e;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    project->current = project->todo_list->last;
    clrk_draw_todos();
  }

  LOG("END");
}

static void clrk_todo_select_first(void)
{
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);
    project->current = project->todo_list->first;
    clrk_draw_todos();
  }

  LOG("END");
}

void clrk_todo_info(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    if (project->current) {
      todo = clrk_list_elem_data(project->current);
      todo->state = (todo->state == INFO)?UNCHECKED:INFO;
      clrk_draw_todos();
    }
  }
  LOG("END");
}

void clrk_todo_running(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    if (project->current) {
      todo = clrk_list_elem_data(project->current);
      todo->state = (todo->state == RUNNING)?UNCHECKED:RUNNING;
      clrk_draw_todos();
    }
  }

  LOG("END");
}

void clrk_init(const char *json)
{
  HERE();
  clerk.current               = NULL;
  clerk.json                  = json;
  clerk.project_list          = malloc(sizeof(clrk_list_t));
  assert(clerk.project_list);
  clerk.project_list->first   = NULL;
  clerk.project_list->last    = NULL;
  clerk.project_list->num_of_elems = 0;

  clrk_draw_init();

  if (!clrk_load()) {
    /* Show help screen */
    clrk_draw_help();
    clrk_draw_status("Couldn't load config");
    /* wait for ANY input key */
    tb_present();
    struct tb_event event;
    while (tb_poll_event(&event)) {
      if (event.type == TB_EVENT_KEY) {
        break;
      }
    }
  }

  if (clerk.project_list->first) {
    clerk.current = clerk.project_list->first;
    clrk_project_t *project = clrk_list_elem_data(clerk.current);
    project->current = project->todo_list->first;
  }

  clrk_draw();
  tb_set_cursor(TB_HIDE_CURSOR, TB_HIDE_CURSOR);

  tb_present();
  LOG("END");
}

void clrk_loop_normal(void)
{
  HERE();
  char *input, last_char_key;
  struct tb_event event;
  clrk_project_t *p;

  while (tb_poll_event(&event)) {
    clrk_draw_todos();
    tb_present();
    if (event.type == TB_EVENT_KEY) {
      /* Go left */
      if (event.key == TB_KEY_ARROW_LEFT
          || event.ch == 'h') {
        LOG(RED"key 'h' or arrow left"NOCOLOR);
        if (clerk.current) {
          clrk_project_set_current(clerk.current->prev);
        }
      /* Go right */
      } else if (event.key == TB_KEY_ARROW_RIGHT
          || event.ch == 'l') {
        LOG(RED"key 'l' or arrow right"NOCOLOR);
        if (clerk.current) {
          clrk_project_set_current(clerk.current->next);
        }
      /* Go down */
      } else if (event.key == TB_KEY_ARROW_DOWN
          || event.ch == 'j') {
        LOG(RED"key 'j' or arrow down"NOCOLOR);
        if (clerk.current) {
          clrk_todo_select_next();
        }
      /* Go up */
      } else if (event.key == TB_KEY_ARROW_UP
          || event.ch == 'k') {
        LOG(RED"key 'k' or arrow up"NOCOLOR);
        if (clerk.current) {
          clrk_todo_select_prev();
        }
      } else {
        switch (event.ch) {
          case 'e':
            /* Edit todo */
            LOG(RED"key 'e'"NOCOLOR);
            clrk_todo_edit_current();
            break;
          case 'E':
            /* Edit project */
            LOG(RED"key 'E'"NOCOLOR);
            clrk_project_edit_current();
            break;
          case 'g':
            /* Select first todo */
            if (last_char_key == 'g') {
              LOG(RED"key 'gg'"NOCOLOR);
              clrk_todo_select_first();
              last_char_key = '\0';
            }
            break;
          case 'G':
            /* Select last todo */
            LOG(RED"key 'G'"NOCOLOR);
            clrk_todo_select_last();
            break;
          case 'H':
            /* Move current project left */
            LOG(RED"key 'K'"NOCOLOR);
            clrk_project_move_left();
            break;
          case 'i':
            /* Mark as todo as info */
            LOG(RED"key 'i'"NOCOLOR);
            clrk_todo_info();
            break;
          case 'J':
            /* Move current todo down */
            LOG(RED"key 'J'"NOCOLOR);
            clrk_todo_move_down();
            break;
          case 'K':
            /* Move current todo down */
            LOG(RED"key 'K'"NOCOLOR);
            clrk_todo_move_up();
            break;
          case 'L':
            /* Move current project right */
            LOG(RED"key 'K'"NOCOLOR);
            clrk_project_move_right();
            break;
          case 'p':
            /* Add new project */
            LOG(RED"key 'p'"NOCOLOR);
            clrk_project_add(NULL);
            break;
          case 'P':
            /* Delete project */
            LOG(RED"key 'P'"NOCOLOR);
            clrk_project_remove_current();
            clrk_draw();
            break;
          case 'Q':
            LOG(RED"key 'Q'"NOCOLOR);
            return;
          case 'r':
            /* Mark current todo as 'running'/'next' */
            LOG(RED"key 'r'"NOCOLOR);
            clrk_todo_running();
            break;
          case 'R':
            LOG(RED"key 'R'"NOCOLOR);
            /* Load json file */
            if (clrk_load()) {
              clrk_draw();
              clrk_draw_status("loaded json");
            } else {
              clrk_draw_status("cannot find json");
            }
            break;
          case 'S':
            LOG(RED"key 'S'"NOCOLOR);
            /* Write projects/todos to json file */
            clrk_save();
            clrk_draw_status("written to json");
            break;
          case 't':
            /* Add new todo */
            LOG(RED"key 't'"NOCOLOR);
            clrk_todo_add(NULL);
            break;
          case 'T':
            /* Add new todo */
            LOG(RED"key 'T'"NOCOLOR);
            clrk_todo_remove_current();
            break;
          case '?':
            LOG(RED"key '?'"NOCOLOR);
            /* Show help box */
            clrk_draw_help();
            tb_present();
            struct tb_event event;
            while (tb_poll_event(&event)) {
              if (event.type == TB_EVENT_KEY) {
                break;
              } else if (event.type == TB_EVENT_RESIZE) {
                if (tb_width() > CLRK_MIN_WIDTH && tb_height() > CLRK_MIN_HEIGHT) {
                  clrk_draw();
                  clrk_draw_help();
                  tb_present();
                }
              }
            }
            clrk_draw_todos();
            break;
          case '0':
            LOG(RED"key '0'"NOCOLOR);
            /* Go to the first project */
            clrk_project_set_current(clerk.project_list->first);
            break;
            case '$':
              /* Go to the last project */
              LOG(RED"key '$'"NOCOLOR);
              clrk_project_set_current(clerk.project_list->last);
              break;
        }
      }

      switch (event.key) {
        case TB_KEY_ESC:
          clrk_draw_todos();
          break;
        case TB_KEY_SPACE:
          clrk_todo_tick_off();
          break;
      }
    } else if (event.type == TB_EVENT_RESIZE) {
      if (tb_width() > CLRK_MIN_WIDTH && tb_height() > CLRK_MIN_HEIGHT) {
        clrk_draw();
      }
    }
    if (event.ch) {
      last_char_key = event.ch;
    }
    tb_present();
  }
  free(clerk.project_list);
}
