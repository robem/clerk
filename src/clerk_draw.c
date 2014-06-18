#include <clerk.h>

extern clrk_clerk_t clerk;

static int itoa(char *buf, unsigned int number)
{
  HERE();
  int i = 0;
  while (number) {
    *buf-- = '0' + (number % 10);
    number /= 10;
    i++;
  }
  return i;
}

unsigned int clrk_draw_text(int x, int y, const char *text, int fg, int bg)
{
  HERE();
  unsigned w = 0, width = tb_width();
  struct tb_cell cells[width];

  while (w < width && *text != '\0') {
    cells[w].ch = *text++;
    cells[w].fg = fg;
    cells[w].bg = bg;
    w++;
  }

  tb_blit(x, y, w, 1, cells);

  return w;
}

int clrk_draw_line(int y, int bg)
{
  HERE();
  int i, width = tb_width();
  struct tb_cell cells[width];

  for (i = 0; i < width; ++i) {
    cells[i].ch = ' ';
    cells[i].fg = 0;
    cells[i].bg = bg;
  }

  tb_blit(0, y, width, 1, cells);

  return width;
}

void clrk_draw_project_line(void)
{
  HERE();
  char name[tb_width()];
  clrk_list_elem_t *project_elem;
  clrk_project_t *project;
  int i, x, length, width, space_per_project, spaces;
  int fg, bg;

  width = tb_width();

  /* Draw background line */
  clrk_draw_line(0, CLRK_COLOR_PRJ_LINE);

  /* Draw number of project in the first field */
#define NUM_BUF_LEN 4
  char num_buf[NUM_BUF_LEN];
  length = itoa(&num_buf[NUM_BUF_LEN - 1], clerk.project_list->num_of_elems);
  for (i = 0; i < length; ++i) {
    tb_change_cell(i, CLRK_DRAW_PRJ_LINE_Y, num_buf[NUM_BUF_LEN - length + i],
        CLRK_COLOR_PRJ_CURRENT, CLRK_COLOR_PRJ_LINE);
  }
#undef NUM_BUF_LEN

  if (clerk.project_list->num_of_elems > 0) {
    fg = CLRK_COLOR_PRJ_FG;
    bg = CLRK_COLOR_PRJ_BG;

    /* Draw project names */
    x = 3;
    LOG("width-3 %d, number of projects %d", width-3, clerk.project_list->num_of_elems);
    space_per_project = (width-3)/clerk.project_list->num_of_elems;
    if (space_per_project < CLRK_PRJ_NAME_SIZE+2) {
       /* Set minimum space per project name */
       space_per_project = CLRK_PRJ_NAME_SIZE+2;
    }

    unsigned visible_projects = (width-3)/space_per_project;
    unsigned index = 0;

    clrk_list_elem_t *start = clerk.project_list->first;
    if (visible_projects < clerk.project_list->num_of_elems) {
       clrk_list_elem_t *elem = clerk.project_list->first;
       clrk_project_t *active_project = clrk_list_elem_data(clerk.current);
       clrk_project_t *p;

       while(elem) {
          index++;
          p = clrk_list_elem_data(elem);
          /* Scroll right/left: common case */
          if (active_project->visible && p->visible) {
            start = elem;
            break;
          }
          /* Scroll left: most left column is active project */
          if (p == active_project && !p->visible && elem->next
              && ((clrk_project_t*)clrk_list_elem_data(elem->next))->visible) {
            start = elem;
            break;
          }
          /* Scroll right: active_project not found yet */
          if (index > visible_projects) {
            start = start->next;
          }
          /* Scroll right: bottom line is active_todo && initial draw */
          if (p == active_project && !p->visible) {
            break;
          }
          elem = elem->next;
       }
    }

    clrk_project_t *start_project = clrk_list_elem_data(start);
    bool start_seen = false;
    LIST_FOREACH(project_elem, clerk.project_list) {
      project = clrk_list_elem_data(project_elem);
      length = strlen(project->name);

      /* Check if we reached start of visible todos */
      if (!start_seen && project == start_project) {
         start_seen = true;
      }

      /* Set todo's visibility */
      if (start_seen && visible_projects) {
         project->visible = true;
         visible_projects--;
      } else {
         project->visible = false;
         continue;
      }

      /* Calculate number of framing spaces */
      spaces = (space_per_project - length) / 2;

      /* Add spaces around the name */
      for (i = 0; i < spaces; ++i) {
        name[i] = ' ';
      }

      strcpy(name + spaces, project->name);

      for (i = 0; i < spaces; ++i) {
        name[spaces + length + i] = ' ';
      }
      name[(2*spaces) + length] = '\0';

      /* Highlight currently selected project */
      if (project == clrk_list_elem_data(clerk.current)) {
        bg = CLRK_COLOR_PRJ_CURRENT;
      } else {
        bg = CLRK_COLOR_PRJ_BG;
      }

      LOG("draw project '%s'", name);
      clrk_draw_text(x, CLRK_DRAW_PRJ_LINE_Y, name, fg, bg);
      x += space_per_project;
    }
  }
  LOG("END");
}

static void clrk_draw_todo_clear(void)
{
  HERE();
  unsigned width, height;
  unsigned i;

  width = tb_width();
  height = tb_height() - (CLRK_DRAW_PRJ_LINE_Y + 1);

  struct tb_cell cells[width * height];
  struct tb_cell cell = {.ch = ' ', .fg = 15, .bg = 16};

  for (i = 0; i < (width * height); ++i) {
    cells[i] = cell;
  }

  tb_blit(0, CLRK_DRAW_PRJ_LINE_Y + 1, width, height, cells);
  LOG("END");
}

void clrk_draw_todos(void)
{
  HERE();
  unsigned x, y;
  clrk_list_elem_t *todo_elem;
  clrk_todo_t *todo;
  clrk_project_t *project;

  clrk_draw_todo_clear();

  if (clerk.current != NULL) {
    project = clrk_list_elem_data(clerk.current);

    if (project && project->todo_list->num_of_elems > 0) {
      unsigned visible_todos = (CLRK_DRAW_STATUS_LINE - CLRK_DRAW_TODO_START_Y) / 3;

      /* Calculate the first todo to draw */
      clrk_list_elem_t *start = project->todo_list->first;
      if (visible_todos < project->todo_list->num_of_elems) {
        unsigned index = 0;
        clrk_list_elem_t *e = start;
        clrk_todo_t *t = NULL;
        clrk_todo_t *active_todo = clrk_list_elem_data(project->current);

        while (e) {
          index++;
          t = clrk_list_elem_data(e);
          /* Scroll upwards/downwards: common case */
          if (active_todo->visible && t->visible) {
            start = e;
            break;
          }
          /* Scroll upwards: top line is active todo */
          if (t == active_todo && !t->visible && e->next
              && ((clrk_todo_t*)clrk_list_elem_data(e->next))->visible) {
            start = e;
            break;
          }
          /* Scroll downwards: active_todo not found yet */
          if (index > visible_todos) {
            start = start->next;
          }
          /* Scroll downwards: bottom line is active_todo && initial draw */
          if (t == active_todo && !t->visible) {
            break;
          }
          e = e->next;
        }
      }

      /* Draw todo_list */
      clrk_todo_t *start_todo = clrk_list_elem_data(start);
      bool start_seen = false;
      x = CLRK_DRAW_TODO_START_X;
      y = CLRK_DRAW_TODO_START_Y;
      LIST_FOREACH(todo_elem, project->todo_list) {
        todo = clrk_list_elem_data(todo_elem);

        /* Check if we reached start of visible todos */
        if (!start_seen && todo == start_todo) {
          start_seen = true;
        }

        /* Set todo's visibility */
        if (start_seen && visible_todos) {
          todo->visible = true;
          visible_todos--;
        } else {
          todo->visible = false;
          continue;
        }

        /* Make checkbox red or green */
        switch (todo->state) {
           case RUNNING:
              clrk_draw_text(x, y, "[*] ", CLRK_COLOR_RUNNING_TRUE, CLRK_COLOR_TODO_BG);
              break;
           case INFO:
              clrk_draw_text(x, y, "[i] ", CLRK_COLOR_INFO_TRUE, CLRK_COLOR_TODO_BG);
              break;
           case CHECKED:
              clrk_draw_text(x, y, "[X] ", CLRK_COLOR_CHECKED_TRUE, CLRK_COLOR_TODO_BG);
              break;
           default:
              clrk_draw_text(x, y, "[ ] ", CLRK_COLOR_CHECKED_FALSE, CLRK_COLOR_TODO_BG);
        }

        /* Highlight background if todo is selected */
        if (todo == clrk_list_elem_data(project->current)) {
          clrk_draw_text(x+4, y, todo->message, CLRK_COLOR_TODO_FG, CLRK_COLOR_TODO_CURRENT);
        } else {
          clrk_draw_text(x+4, y, todo->message, CLRK_COLOR_TODO_FG, CLRK_COLOR_TODO_BG);
        }
        LOG("\tdraw todo: x=%d, y=%d, msg=%s, visible=%s, visible_todos=%d", x+4, y, todo->message, todo->visible?"true":"false", visible_todos);
        y += 3;
      }
    }
  }
  LOG("END");
}

void clrk_draw_show_input_line(void)
{
  unsigned height = tb_height();

  /* Draw background command line */
  clrk_draw_line(height - 1, CLRK_COLOR_INPUT_BG);

  /* Draw prompt */
  clrk_draw_text(0, height - 1, "> ", CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);

  /* Set cursor */
  tb_set_cursor(2, height - 1);
}

void clrk_draw_remove_input_line(void)
{
  unsigned height = tb_height();
  /* Remove command line */
  clrk_draw_line(height - 1, CLRK_COLOR_TODO_BG);

  /* Unset cursor */
  tb_set_cursor(TB_HIDE_CURSOR, TB_HIDE_CURSOR);
}

void clrk_draw_status(const char* status)
{
  unsigned height = tb_height();
  const char *prompt = " clerk ";

  /* Draw background command line */
  clrk_draw_line(height - 1, CLRK_COLOR_INPUT_BG);

  /* Draw prompt */
  clrk_draw_text(0, height - 1, prompt, CLRK_COLOR_PROMPT_FG, CLRK_COLOR_PROMPT_BG);

  /* Draw status message */
  clrk_draw_text(0 + strlen(prompt), height - 1, " ", CLRK_COLOR_INPUT_FG | TB_BOLD, CLRK_COLOR_INPUT_BG);
  clrk_draw_text(0 + strlen(prompt) + 1, height - 1, status, CLRK_COLOR_INPUT_FG | TB_BOLD, CLRK_COLOR_INPUT_BG);
}

void clrk_draw_help(void)
{
  HERE();
  unsigned height = tb_height();
  unsigned width  = tb_width();
  unsigned i = 0;

  /* TODO: Keys should be obtained from a config file */
  /* Define help text */
  const char *help_text[] = {
    "",
    "Keys",
    "====",
    "",
    "Movement keys",
    "h:  Select previous project",
    "l:  Select next project",
    "j:  Select next todo",
    "k:  Select previous todo",
    "",
    "0:  Jump to first project",
    "$:  Jump to last project",
    "gg: Jump to first todo",
    "G:  Jump to last todo",
    "",
    "Action keys",
    "p:  Create new project",
    "E:  Edit current project",
    "P:  Delete current project",
    "t:  Create new todo",
    "e:  Edit current todo",
    "T:  Delete current todo",
    "",
    "J:  Move todo down",
    "K:  Move todo up",
    "H:  Move project left",
    "L:  Move project right",
    "",
    "Space: tick off todo",
    "r:  '*' active todo (not saved)",
    "i:  'i' info todo (not saved)",
    "",
    "ESC: Cancel input",
    "",
    "?:  Help",
    "R:  Load json config",
    "S:  Save",
    "Q:  Quit clerk",
    "",
    "Press any key to continue...",
    "END OF HELP"
  };

  /* Calculate number of lines */
  unsigned lines = 0;
  while (strcmp(help_text[i++], "END OF HELP")) {
    lines++;
  }

  if (lines >= height) {
    clrk_draw_status("Window too small to draw help box.");
    return;
  }

  /* Draw a box in the bottom of the screen */
  unsigned start_line = (height - 1) - lines;
  unsigned text_height = height - start_line;
  struct tb_cell cells[width * text_height];

  /* Craft text box and fill it with background color */
  for (i = 0; i < (width * text_height); ++i) {
    cells[i].ch = ' ';
    cells[i].fg = CLRK_COLOR_INPUT_FG;
    cells[i].bg = CLRK_COLOR_INPUT_BG;
  }

  /* Draw text box */
  tb_blit(0, start_line, width, text_height, cells);

  /* Draw help text */
  for (i = 0; i < lines; ++i) {
    clrk_draw_text(1, start_line + i, help_text[i], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
  }

  LOG("END");
}

void clrk_draw(void) {
  HERE();
  clrk_draw_project_line();
  clrk_draw_todos();
  LOG("END");
}

