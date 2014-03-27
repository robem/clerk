#include <clerk.h>

clrk_clerk_t clerk;

static char* clrk_input(char *text)
{
  HERE();
  unsigned i, cx, cy;
  char *buffer = malloc(CLRK_INPUT_BUFFER_SIZE);
  struct tb_event event;

  cy = tb_height() - 1;
  cx = 3;
  i = 0;
  if (text) {
    strncpy(buffer, text, CLRK_INPUT_BUFFER_SIZE);
    i = strlen(text);
    cx += strlen(text);
    clrk_draw_text(2, cy, text, 15, CLRK_COLOR_INPUT_BG);
    tb_set_cursor(2 + strlen(text), cy);
    tb_present();
  }
  while (tb_poll_event(&event)) {
    if (event.type == TB_EVENT_KEY) {
      if (event.key == TB_KEY_ESC) {
        break;
      } else if (event.key == TB_KEY_ENTER) {
        if (i < CLRK_INPUT_BUFFER_SIZE) {
          buffer[i] = '\0';
        }
        LOG("END");
        return buffer;
      } else if (event.key == TB_KEY_BACKSPACE || event.key == TB_KEY_BACKSPACE2) {
        if (i > 0) {
          tb_change_cell(2 + (--i), tb_height() - 1, ' ', 15, CLRK_COLOR_INPUT_BG);
          buffer[i] = '\0';
          tb_set_cursor((cx--)-2, cy);
        }
      } else if (event.ch > 31 && event.ch < 127) {
        LOG("input key %c", event.ch);
        if (i < CLRK_INPUT_BUFFER_SIZE) {
          tb_change_cell(2 + i, tb_height() - 1, event.ch, 15, CLRK_COLOR_INPUT_BG);
          buffer[i++] = event.ch;
          tb_set_cursor(cx++, cy);
        }
      } else if (event.key == TB_KEY_SPACE) {
        if (i < CLRK_INPUT_BUFFER_SIZE) {
          tb_change_cell(2 + i, tb_height() - 1, ' ', 15, CLRK_COLOR_INPUT_BG);
          buffer[i++] = ' ';
          tb_set_cursor(cx++, cy);
        }
      }
      LOG("key %d", event.key);
    } else if (event.type == TB_EVENT_RESIZE) {
    }
    tb_present();
  }

  return NULL;
}

static clrk_project_t* clrk_project_new(const char *name)
{
  HERE();
  clrk_project_t *project;
  project = (clrk_project_t*)malloc(sizeof(clrk_project_t));

  assert(project);

  memset((void*)project->name, 0, CLRK_PRJ_NAME_SIZE);
  if (name) {
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

  clrk_draw_show_input_line();
  tb_present();

  if (name == NULL) {
    input = clrk_input(NULL);
    clrk_draw_remove_input_line();
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
    clrk_draw_project_line();
    clrk_draw_todos();
  }
  LOG("lproj "PTR, project);
  LOG("END");
  return project;
}

void clrk_project_remove_current(void)
{
  HERE();
  int i;
  clrk_list_t *destroy_me;
  clrk_project_t *project;

  if (clerk.project_list == NULL) {
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

  clrk_draw_project_line();
  clrk_draw_todos();
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
    clrk_draw_show_input_line();
    tb_present();
    input = clrk_input(NULL);
    clrk_draw_remove_input_line();
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

      clrk_draw_show_input_line();
      tb_present();
      input = clrk_input(todo->message);
      if (input == NULL) {
         return;
      }
      clrk_draw_remove_input_line();
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

  /* TODO: Load json data */
  clrk_load();

  clrk_draw_project_line();

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
    if (event.type == TB_EVENT_KEY) {
      switch (event.ch) {
        case 'Q':
          LOG(RED"key 'Q'"NOCOLOR);
          return;
        case 'P':
          /* Delete project */
          LOG(RED"key 'P'"NOCOLOR);
          clrk_project_remove_current();
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
        case 'h':
          /* Go left */
          LOG(RED"key 'h'"NOCOLOR);
          if (clerk.current) {
            clrk_project_set_current(clerk.current->prev);
          }
          break;
        case 'l':
          LOG(RED"key 'l'"NOCOLOR);
          /* Go right */
          if (clerk.current) {
            clrk_project_set_current(clerk.current->next);
          }
          break;
        case 'j':
          LOG(RED"key 'j'"NOCOLOR);
          /* Go down */
          clrk_todo_next();
          break;
        case 'k':
          LOG(RED"key 'k'"NOCOLOR);
          /* Go down */
          clrk_todo_prev();
          break;
        case 'S':
          clrk_save();
          clrk_draw_status("written to "CLRK_CONFIG_FILE);
          break;
        /* case 'L': */
        /*   clrk_load(); */
        /*   clrk_draw_status(CLRK_CONFIG_FILE" loaded"); */
        /*   break; */
        case '0':
          /* Go to the first project */
          LOG(RED"key '0'"NOCOLOR);
          clrk_project_set_current(clerk.project_list);
          break;
          /* case '$': */
          /*   #<{(| Go to the last project |)}># */
          /*   LOG(RED"key '$'"NOCOLOR); */
          /*   clrk_project_set_current(clerk.project_list->prev); */
          /*   break; */
      }

      switch (event.key) {
        case TB_KEY_SPACE:
          clrk_todo_tick_off();
          break;
      }
    } else if (event.type == TB_EVENT_RESIZE) {
    }
    tb_present();
  }
}
