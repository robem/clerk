#include <clerk.h>

clrk_clerk_t clerk;

static void clrk_set_status_and_await_key_press(const char *status)
{
  clrk_draw_status(status);
  /* wait for ANY input key */
  tb_present();
  struct tb_event event;
  while (tb_poll_event(&event)) {
    if (event.type == TB_EVENT_KEY) {
      break;
    }
  }
}

static char * clrk_input(char *text)
{
  HERE();
  unsigned buffer_idx, buffer_size, text_len, cx, cy;
  struct tb_event event;
  bool space = false;
  char *buffer = NULL;

  cy = CLRK_DRAW_STATUS_LINE;   /* Y: input line */
  cx = CLRK_DRAW_INPUT_START_X; /* X: start with an offset */

  clrk_draw_show_input_line();

  /* Draw given text and set cursor at the end */
  if (text) {
    LOG("Use provided text");
    text_len = strlen(text);
    buffer = malloc(text_len);
    buffer_size = text_len + 1;
    /* XXX sanity check for string length */
    strncpy(buffer, text, buffer_size);
    clrk_draw_text(cx, cy, text, 15, CLRK_COLOR_INPUT_BG);
    cx += text_len;
    tb_set_cursor(cx, cy);
  } else {
    LOG("Create empty buffer");
    text_len = 0;
    buffer = malloc(CLRK_INPUT_INIT_BUFFER_SIZE);
    buffer_size = CLRK_INPUT_INIT_BUFFER_SIZE;
    memset((void*)buffer, '\0', buffer_size);
  }

  tb_present();

  /* Start input loop */
  while (tb_poll_event(&event)) {
    buffer_idx = cx - CLRK_DRAW_INPUT_START_X;
    if (event.type == TB_EVENT_KEY) {
      space = (event.key == TB_KEY_SPACE);

      if (event.key == TB_KEY_ESC) {
        break;
      } else if (event.key == TB_KEY_ENTER) {
        clrk_draw_remove_input_line();
        LOG("END; return buffer \"%s\" @ %p", buffer, buffer);
        return buffer;
      } else if (event.key == TB_KEY_BACKSPACE || event.key == TB_KEY_BACKSPACE2) {
        /* Remove character */
        if (buffer_idx > 0) {
          size_t tail = strlen(&buffer[buffer_idx]);
          if (tail) {
            /* Remove in between */
            LOG("copy tail [%zu]", tail);
            /* Shift tail by 1 to the left */
            char *helper = strdup(&buffer[buffer_idx]);
            memcpy(&buffer[buffer_idx-1], helper, strlen(helper));
            free(helper);
            /* Paint over last character */
            buffer[buffer_idx+tail-1] = ' ';
            clrk_draw_text(cx - 1, cy, &buffer[buffer_idx-1], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
            /* Clear last character */
            buffer[buffer_idx+tail-1] = '\0';
          } else {
            LOG("remove last char");
            buffer[buffer_idx-1] = '\0';
            tb_change_cell(cx - 1, cy, '\0', CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
          }
          text_len--;
          tb_set_cursor(--cx, cy);
        }
      } else if ((event.ch > 31 && event.ch < 127) || space) {
        /* Add character */
        LOG("input [%d] key %c", buffer_idx, event.ch);
add_char:
        if ((buffer_idx < buffer_size) && (text_len < (buffer_size-1))) {
          if (buffer[buffer_idx] == '\0') {
            LOG("Append character");
            /* Append */
            tb_change_cell(cx, clerk.height - 1, event.ch, CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
            buffer[buffer_idx] = space ? ' ' : event.ch;
          } else {
            /* Add character in between */
            LOG("Add character in between");
            char *helper = strdup(&buffer[buffer_idx]);
            memcpy(&buffer[buffer_idx+1], helper, strlen(helper));
            free(helper);
            buffer[buffer_idx] = space ? ' ' : event.ch;
            clrk_draw_text(cx, cy, &buffer[buffer_idx], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
          }
          text_len++;
          tb_set_cursor(++cx, cy);
        } else {
          /* Increase input buffer */
          LOG("Increase input buffer");
          buffer = realloc(buffer, buffer_size + CLRK_INPUT_INIT_BUFFER_SIZE);
          memset(buffer + buffer_size, '\0', CLRK_INPUT_INIT_BUFFER_SIZE);
          buffer_size += CLRK_INPUT_INIT_BUFFER_SIZE;
          if (buffer == NULL) {
            LOG("ERROR: Couldn't realloc input buffer");
            clrk_draw_remove_input_line();
            clrk_draw_status("Memory error: Couldn't extend input buffer.");
            tb_present();
            return NULL;
          }
          goto add_char;
        }
      } else if (event.key == TB_KEY_ARROW_LEFT) {
        if (buffer_idx > 0) {
          tb_set_cursor(--cx, cy);
        }
      } else if (event.key == TB_KEY_ARROW_RIGHT) {
        if (buffer_idx < buffer_size && buffer[buffer_idx] != '\0') {
          tb_set_cursor(++cx, cy);
        }
      } else if (event.key == TB_KEY_HOME) {
        cx = CLRK_DRAW_INPUT_START_X;
        tb_set_cursor(cx, cy);
      } else if (event.key == TB_KEY_END) {
        cx = CLRK_DRAW_INPUT_START_X + text_len;
        tb_set_cursor(cx, cy);
      }
      LOG("key %d", event.key);
    } else if (event.type == TB_EVENT_RESIZE) {
    }
    tb_present();
  }

  clrk_draw_remove_input_line();
  return NULL;
}

static clrk_project_t * clrk_project_new(const char *name)
{
  HERE();
  clrk_project_t *project = malloc(sizeof(clrk_project_t));
  assert(project);
  assert(name);

  project->name = strdup(name);
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
  clrk_project_t *project = NULL;
  char *buffer = NULL;

  if (name == NULL) {
    /* Create projects interactively */
    buffer = clrk_input(NULL);
  } else {
    /* Create projects while parsing json file */
    buffer = malloc(strlen(name) + 1);
    strncpy(buffer, name, strlen(name) + 1);
  }

  if (buffer != NULL) {
     project = clrk_project_new(buffer);

     free(buffer);

     elem = clrk_list_add(clerk.project_list, project);
     clerk.current = elem;

     clrk_draw();
  }

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
  char *buffer = NULL;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    buffer = clrk_input(project->name);
    if (buffer == NULL || buffer[0] == '\0') {
      /* No empty string permitted */
      goto end;
    }

    if (strlen(project->name) != strlen(buffer)) {
      /* Don't waste memory or extend if necessary*/
      project->name = realloc(project->name, strlen(buffer) + 1);
    }
    strncpy(project->name, buffer, strlen(buffer) + 1);
    clrk_draw_project_line();
  }

end:
  free(buffer);
}

void clrk_project_remove_current(void)
{
  HERE();
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
  clrk_todo_t *todo;

  assert(text);

  todo = (clrk_todo_t*)malloc(sizeof(clrk_todo_t));
  assert(todo);

  /* XXX sanity check for strlen */
  todo->message = malloc(strlen(text) + 1);
  strncpy(todo->message, text, strlen(text) + 1);
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
  clrk_todo_t *todo = NULL;
  clrk_list_elem_t *elem;
  char *buffer = NULL;

  assert(clerk.project_list);
  if (clerk.project_list->num_of_elems <= 0) {
    goto out;
  }

  project = clrk_list_elem_data(clerk.current);

  if (text == NULL) {
    buffer = clrk_input(NULL);
  } else {
    buffer = malloc(strlen(text) + 1);
    strncpy(buffer, text, strlen(text) + 1);
  }

  /* Empty strings are not permitted. */
  if (buffer == NULL || buffer[0] == '\0') {
    goto out;
  }

  todo = clrk_todo_new(buffer);
  assert(todo);

  free(buffer);

  elem = clrk_list_add(project->todo_list, todo);
  assert(elem);

  project->current = elem;
  clrk_draw_todos();

out:
  LOG("END");
  return todo;
}

static void clrk_todo_edit_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  char *buffer = NULL;

  if (clerk.current) {
    project = clrk_list_elem_data(clerk.current);

    if (project->current) {
      todo = clrk_list_elem_data(project->current);

      buffer = clrk_input(todo->message);
      if (buffer == NULL || buffer[0] == '\0') {
        /* Deleting the whole todo text is not permitted. Use 'T' instead. */
        goto end;
      }

      if (strlen(todo->message) != strlen(buffer)) {
        /* Don't waste memory or extend if necessary*/
        todo->message = realloc(todo->message, strlen(buffer) + 1);
      }
      strncpy(todo->message, buffer, strlen(buffer) + 1);
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
  clrk_project_t *project = NULL;

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
  clrk_project_t *project = NULL;

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
  clrk_project_t *project = NULL;

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
  clrk_project_t *project = NULL;

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

/*
 * Wrapper around the supplied exit function that sets a flag to indicate to
 * the main loop to exit.
 *
 */
void clrk_shutdown(void)
{
  clerk.exit_func_invoked = true;
  clerk.exit_func();
}

void clrk_init(const char *json, const char *config, clrk_exit_func exit_func)
{
  HERE();
  clerk.current               = NULL;
  clerk.json                  = json;
  clerk.project_list          = malloc(sizeof(clrk_list_t));
  assert(clerk.project_list);
  clerk.project_list->first   = NULL;
  clerk.project_list->last    = NULL;
  clerk.project_list->num_of_elems = 0;
  clerk.config                   = config;
  clerk.colors                   = malloc(sizeof(color_configuration_t));
  /* Set all colors to -1 due 0 is a valid color */
  clerk.colors->bg               = -1;
  clerk.colors->project_fg       = -1;
  clerk.colors->project_bg       = -1;
  clerk.colors->project_selected = -1;
  clerk.colors->todo_fg          = -1;
  clerk.colors->todo_bg          = -1;
  clerk.colors->todo_selected    = -1;
  clerk.colors->todo             = -1;
  clerk.colors->done             = -1;
  clerk.colors->star             = -1;
  clerk.colors->info             = -1;
  clerk.colors->prompt_fg        = -1;
  clerk.colors->prompt_bg        = -1;
  clerk.colors->input_fg         = -1;
  clerk.colors->input_bg         = -1;
  clerk.width                    = tb_width();
  clerk.height                   = tb_height();
  clerk.exit_func                = exit_func;
  clerk.exit_func_invoked        = false;

  if (!clrk_read_config()) {
    /* Show help screen */
    clrk_draw_help();
    clrk_set_status_and_await_key_press("Couldn't load config");
  }

  clrk_draw_init();

  if (!clrk_load()) {
    /* Show help screen */
    clrk_draw_help();
    clrk_set_status_and_await_key_press("Couldn't load todos");
  }

  if (clerk.project_list->first) {
    clerk.current = clerk.project_list->first;
    clrk_project_t *project = clrk_list_elem_data(clerk.current);
    project->current = project->todo_list->first;
  }

  if (!clrk_sig_init(&clrk_shutdown)) {
    clrk_set_status_and_await_key_press("Couldn't install signal handler");
  }

  clrk_draw();
  tb_set_cursor(TB_HIDE_CURSOR, TB_HIDE_CURSOR);

  tb_present();
  LOG("END");
}

bool clrk_loop_normal(void)
{
  HERE();
  char last_char_key;
  struct tb_event event;

  while (tb_poll_event(&event)) {
    /*
     * When clerk is terminated by a signal, i.e., through invocation of the
     * exit function, the 'exit_func_invoked' flag will be set in which case
     * there is no need to try processing the event.
     */
    if (clerk.exit_func_invoked) {
      break;
    }

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
          case 'C':
            LOG(RED"key 'C'"NOCOLOR);
            /* Load config file */
            if (clrk_read_config()) {
              clrk_draw();
              clrk_draw_status("loaded config");
            } else {
              clrk_draw_status("couldn't load config");
            }
            break;
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
            goto exit;
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
              clrk_draw_status("couldn't load json");
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
                LOG("resize 1 cw %d ch %d ew %d eh %d", clerk.width, clerk.height, event.w, event.h);
                clerk.width = event.w;
                clerk.height = event.h;
                if (clerk.width > CLRK_MIN_WIDTH && clerk.height > CLRK_MIN_HEIGHT) {
                  tb_clear();
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
      LOG("resize 2 cw %d ch %d ew %d eh %d", clerk.width, clerk.height, event.w, event.h);
      clerk.width = event.w;
      clerk.height = event.h;
      if (clerk.width > CLRK_MIN_WIDTH && clerk.height > CLRK_MIN_HEIGHT) {
         /*
          * Somehow termbox needs to clear the intenal back buffer to hanlde resize correctly
          * although tb_present was called in the last loop
          */
        tb_clear();
        clrk_draw();
      }
    }
    if (event.ch) {
      last_char_key = event.ch;
    }
    tb_present();
  }

exit:
  /* Deinit */
  free(clerk.project_list);
  free(clerk.colors);
  return clerk.exit_func_invoked;
}
