#include <clerk.h>

clrk_clerk_t clerk;

static char* clrk_input(char *text)
{
  HERE();
  unsigned i, cx, cy;
  struct tb_event event;
  bool space = false;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);

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
        if (i < CLRK_INPUT_BUFFER_SIZE) {
          buffer[i] = '\0';
        }
        clrk_draw_remove_input_line();
        LOG("END; return buffer \"%s\" @ %p", buffer, buffer);
        return buffer;
      } else if (event.key == TB_KEY_BACKSPACE || event.key == TB_KEY_BACKSPACE2) {
        if (i > 0) {
          size_t tail = strlen(&buffer[i]);
          if (tail) {
            LOG("copy tail [%zu]", tail);
            /* Shift tail by 1 to the left */
            strcpy(&buffer[i-1], &buffer[i]);
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
            strcpy(&buffer[i+1], &buffer[i]);
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
  return NULL;
}

static clrk_project_t* clrk_project_new(const char *name)
{
  HERE();
  clrk_project_t *project;
  project = (clrk_project_t*)malloc(sizeof(clrk_project_t));

  assert(project);

  memset((void*)project->name, 0, CLRK_PRJ_NAME_SIZE);
  if (name && *name != '\0') {
    strncpy(project->name, name, CLRK_PRJ_NAME_SIZE);
  } else {
    strncpy(project->name, "NONAME", CLRK_PRJ_NAME_SIZE);
  }
  memset((void*)name, 0, CLRK_PRJ_NAME_SIZE);

  project->todo_list = NULL;
  project->current = NULL;
  project->number_of_todos = 0;

  LOG("add new project: %s "PTR, project->name, project);

  LOG("END");
  return project;
}

clrk_project_t* clrk_project_add(const char *name)
{
  HERE();
  clrk_list_t *elem;
  clrk_project_t *project;
  char *input;

  if (name == NULL) {
    input = clrk_input(NULL);
    if (input == NULL) {
      return NULL;
    }
  } else {
    input = (char*)name;
  }

  project = clrk_project_new(input);

  if (name == NULL) {
    free(input);
  }

  LOG("%s current lproject "PTR, name, clerk.current);
  elem = clrk_list_add(&clerk.project_list, project);
  assert(elem);

  /* clerk.project_list->prev = elem; */

  /* if (clerk.current == NULL) { */
  /*   clerk.current = clerk.project_list; */
  /* } */
  clerk.current = elem;
  LOG("current lproject "PTR, clerk.current);

  clerk.number_of_projects++;
  clrk_draw_project_line();

  LOG("after draw line");

  LOG("END");
  return project;
}

clrk_list_t* clrk_project_set_current(clrk_list_t *project)
{
  HERE();
  if (project) {
    clerk.current = project;
    clrk_draw();
  }
  LOG("lproj "PTR, project);
  LOG("END");
  return project;
}

static void clrk_project_edit_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  char *input;

  if (clerk.current) {
    project = clrk_list_data(clerk.current);

    input = clrk_input(project->name);
    if (input == NULL || input == '\0') {
      return;
    }
    strncpy(project->name, input, CLRK_PRJ_NAME_SIZE);
    clrk_draw_project_line();
  }
}

void clrk_project_remove_current(void)
{
  HERE();
  int i;
  clrk_list_t *destroy_me;
  clrk_project_t *project;

  if (clerk.project_list == NULL || clerk.current == NULL) {
    return;
  }

  destroy_me = clerk.current;
  project = clrk_list_data(clerk.current);

  /* Remove from project list */
  clrk_list_remove(destroy_me);

  /* Remove all todos of that project */
  clrk_list_t *helper;
  clrk_list_t *todo = project->todo_list;
  while (todo) {
    helper = todo;
    clrk_list_free(todo);
    todo = helper->next;
  }
  project->number_of_todos = 0;

  LOG("old lproj "PTR, clerk.current);
  if (clerk.current == clerk.project_list) {
    /* If we remove list head then set head to NULL or next */
    clerk.project_list = NULL;
    if (clerk.number_of_projects > 0) {
      clerk.project_list = clerk.current->next;
    }
    clerk.current = clerk.project_list;
  } else {
    /* Set current to next or prev of removed element */ 
    if (clerk.current->next) {
      clerk.current = clerk.current->next;
    } else {
      clerk.current = clerk.current->prev;
    }
  }
  LOG("current lproj "PTR, clerk.current);

  /* Destroy element */
  clrk_list_free(destroy_me);

  clerk.number_of_projects--;

  LOG("END");
}

static clrk_todo_t* clrk_todo_new(const char *text)
{
  HERE();
  clrk_todo_t *todo;

  todo = (clrk_todo_t*)malloc(sizeof(clrk_todo_t));
  assert(todo);

  strncpy(todo->message, text, CLRK_TODO_MESSAGE_SIZE);
  todo->checked = false;
  todo->running = false;

  LOG("Add new todo: %s", text);

  LOG("END");
  return todo;
}

clrk_todo_t* clrk_todo_add(const char *text)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  clrk_list_t *elem;
  char *input;

  if (clerk.project_list == NULL) {
    return NULL;
  }

  project = clrk_list_data(clerk.current);

  if (text == NULL) {
    input = clrk_input(NULL);
    if (input == NULL) {
      return NULL;
    }
  } else {
    input = (char*)text;
  }

  todo = clrk_todo_new(input);
  assert(todo);

  if (text == NULL) {
    free(input);
  }

  elem = clrk_list_add(&project->todo_list, todo);
  assert(elem);

  /* if (project->current == NULL) { */
  /*   project->current = elem; */
  /* } */
  project->current = elem;
  LOG("proj "PTR" ltodo "PTR, project, elem);

  project->number_of_todos++;
  clrk_draw_todos();

  LOG("END");
  return todo;
}

static void clrk_todo_edit_current(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;
  char *input;

  if (clerk.current) {
    project = clrk_list_data(clerk.current);

    if (project->current) {
      todo = clrk_list_data(project->current);

      input = clrk_input(todo->message);
      if (input == NULL || input == '\0') {
        return;
      }
      strncpy(todo->message, input, CLRK_TODO_MESSAGE_SIZE);
      clrk_draw_todos();
    }
  }
}

static void clrk_todo_remove_current(void)
{
  HERE();
  clrk_project_t *project;

  if (clerk.current) {
    project = clrk_list_data(clerk.current);

    if (project->current) {
      clrk_list_remove(project->current);
      clrk_list_free(project->current);

      project->number_of_todos--;

      if (project->current == project->todo_list) {
        project->todo_list = NULL;
        if (project->current->next) {
          project->todo_list = project->current->next;
        }
      }
      project->current = project->todo_list;
      LOG("proj "PTR" ltodo "PTR, project, project->current);

      LOG("END");
      clrk_draw_todos();
    }
  }
}

static clrk_list_t* clrk_todo_next(void) {
  HERE();
  clrk_project_t *project;
  if (clerk.current) {
    project = clrk_list_data(clerk.current);
    if (project && project->current && project->current->next) {
      project->current = project->current->next;
      clrk_draw_todos();
    }
  }
  LOG("current todo "PTR, project->current);
  LOG("END");
  return project->current;
}

static clrk_list_t* clrk_todo_prev(void)
{
  HERE();
  clrk_project_t *project;
  if (clerk.current) {
    project = clrk_list_data(clerk.current);
    if (project && project->current && project->current->prev) {
      project->current = project->current->prev;
      clrk_draw_todos();
    }
  }
  LOG("current todo "PTR, project->current);
  LOG("END");
  return project->current;
}

void clrk_todo_tick_off(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_data(clerk.current);

    if (project->current) {
      todo = clrk_list_data(project->current);
      todo->checked = todo->checked ? false : true;
      todo->running = false;
      clrk_draw_todos();
    }
  }
  LOG("END");
}

static void clrk_todo_running(void)
{
  HERE();
  clrk_project_t *project;
  clrk_todo_t *todo;

  if (clerk.current) {
    project = clrk_list_data(clerk.current);

    if (project->current) {
      todo = clrk_list_data(project->current);
      todo->running = todo->running ? false : true;
      todo->checked = false;
      clrk_draw_todos();
    }
  }
  LOG("END");
}

void clrk_init(void)
{
  HERE();
  clerk.project_list        = NULL;
  clerk.current             = NULL;
  clerk.number_of_projects  = 0;

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

  clrk_draw();
  tb_set_cursor(TB_HIDE_CURSOR, TB_HIDE_CURSOR);

  LOG("END");
  tb_present();
}

void clrk_loop_normal(void)
{
  HERE();
  char *input;
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
          clrk_todo_next();
        }
      /* Go up */
      } else if (event.key == TB_KEY_ARROW_UP
          || event.ch == 'k') {
        LOG(RED"key 'k' or arrow up"NOCOLOR);
        if (clerk.current) {
          clrk_todo_prev();
        }
      } else {
        switch (event.ch) {
          case 'Q':
            LOG(RED"key 'Q'"NOCOLOR);
            return;
          case 'P':
            /* Delete project */
            LOG(RED"key 'P'"NOCOLOR);
            clrk_project_remove_current();
            clrk_draw();
            break;
          case 'p':
            /* Add new project */
            LOG(RED"key 'p'"NOCOLOR);
            clrk_project_add(NULL);
            break;
          case 'T':
            /* Add new todo */
            LOG(RED"key 'T'"NOCOLOR);
            clrk_todo_remove_current();
            break;
          case 't':
            /* Add new todo */
            LOG(RED"key 't'"NOCOLOR);
            clrk_todo_add(NULL);
            break;
          case 'E':
            /* Edit project */
            LOG(RED"key 'E'"NOCOLOR);
            clrk_project_edit_current();
            break;
          case 'e':
            /* Edit todo */
            LOG(RED"key 'e'"NOCOLOR);
            clrk_todo_edit_current();
            break;
          case 'r':
            /* Mark current todo as 'running'/'next' */
            LOG(RED"key 'r'"NOCOLOR);
            clrk_todo_running();
            break;
          case 'S':
            LOG(RED"key 'S'"NOCOLOR);
            /* Write projects/todos to json file */
            clrk_save();
            clrk_draw_status("written to json");
            break;
          case 'L':
            LOG(RED"key 'L'"NOCOLOR);
            /* Load json file */
            if (clrk_load()) {
              clrk_draw();
              clrk_draw_status("loaded json");
            } else {
              clrk_draw_status("cannot find json");
            }
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
            clrk_project_set_current(clerk.project_list);
            break;
            /* case '$': */
            /*   #<{(| Go to the last project |)}># */
            /*   LOG(RED"key '$'"NOCOLOR); */
            /*   clrk_project_set_current(clerk.project_list->prev); */
            /*   break; */
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
    tb_present();
  }
}
