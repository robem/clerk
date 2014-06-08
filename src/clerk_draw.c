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
  clrk_project_t *project;
  int i, x, length, width, space_per_project, spaces;
  int fg, bg;

  width = tb_width();

  /* Draw background line */
  clrk_draw_line(0, CLRK_COLOR_PRJ_LINE);

  /* Draw number of project in the first field */
#define NUM_BUF_LEN 4
  char num_buf[NUM_BUF_LEN];
  length = itoa(&num_buf[NUM_BUF_LEN - 1], clerk.number_of_projects);
  for (i = 0; i < length; ++i) {
    tb_change_cell(i, CLRK_DRAW_PRJ_LINE_Y, num_buf[NUM_BUF_LEN - length + i],
        CLRK_COLOR_PRJ_CURRENT, CLRK_COLOR_PRJ_LINE);
  }
#undef NUM_BUF_LEN

  if (clerk.project_list) {
    fg = CLRK_COLOR_PRJ_FG;
    bg = CLRK_COLOR_PRJ_BG;

    LOG("before foreach");
    /* Draw project names */
    x = 3;
    LOG("width-3 %d, cnop %d", width-3, clerk.number_of_projects);
    space_per_project = (width-3)/clerk.number_of_projects;

    FOR_EACH(project, clerk.project_list) {
      LOG("proj name %s", project->name);
      length = strlen(project->name);

      if (space_per_project < (length + 2)) {
        space_per_project = length + 2;
      }
      spaces = (space_per_project - length) / 2;

      LOG("spaces %d, spp %d, length %d", spaces, space_per_project, length);
      /* Add spaces around the name */
      for (i = 0; i < spaces; ++i) {
        name[i] = ' ';
      }

      strcpy(name + spaces, project->name);

      for (i = 0; i < spaces; ++i) {
        name[spaces + length + i] = ' ';
      }
      name[(2*spaces) + length] = '\0';

      LOG("#%s#",name);

      /* Highlight currently selected project */
      if (project == clrk_list_data(clerk.current)) {
        bg = CLRK_COLOR_PRJ_CURRENT;
      } else {
        bg = CLRK_COLOR_PRJ_BG;
      }

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
  clrk_todo_t *todo;
  clrk_project_t *project;

  clrk_draw_todo_clear();

  if (clerk.current != NULL) {
    project = clrk_list_data(clerk.current);

    if (project && project->todo_list) {
      /* LOG("proj->todo_list %p", project->todo_list); */

      unsigned visible_todos = (CLRK_DRAW_STATUS_LINE - CLRK_DRAW_TODO_START_Y) / 3;
      LOG("visible_todos = %d, project->number_of_todos = %d", visible_todos, project->number_of_todos);

      /* Calculate the first todo to draw */
      clrk_list_t *start = project->todo_list;
      if (visible_todos < project->number_of_todos) {
        unsigned index = 0;
        clrk_list_t *e = start;
        clrk_todo_t *t = NULL;
        clrk_todo_t *active_todo = clrk_list_data(project->current);
        LOG("before loop");
        while (e) {
          index++;
          t = clrk_list_data(e);
          /* Scroll upwards/downwards: common case */
          if (active_todo->visible && t->visible) {
            start = e;
            LOG("scroll 1: active \"%s\", t \"%s\"", active_todo->message, t->message);
            break;
          }
          /* Scroll upwards: top line is active todo */
          if (t == active_todo && !t->visible && e->next
              && ((clrk_todo_t*)clrk_list_data(e->next))->visible) {
            start = e;
            LOG("scroll 2: active \"%s\", t \"%s\"", active_todo->message, t->message);
            break;
          }
          /* Scroll downwards: active_todo not found yet */
          if (index > visible_todos) {
            start = start->next;
            LOG("scroll: next");
          }
          /* Scroll downwards: bottom line is active_todo && initial draw */
          if (t == active_todo && !t->visible) {
            LOG("scroll 3: active \"%s\", t \"%s\"", active_todo->message, t->message);
            break;
          }
          e = e->next;
        }
      }

      /* Draw todo_list */
      LOG("eoLoop");
      clrk_todo_t *start_todo = clrk_list_data(start);
      bool start_seen = false;
      x = CLRK_DRAW_TODO_START_X;
      y = CLRK_DRAW_TODO_START_Y;
      FOR_EACH(todo, project->todo_list) {
        LOG("I'M in foreach todo: %s", todo->message);

        if (!start_seen && todo == start_todo) {
          start_seen = true;
        }

        if (start_seen && visible_todos) {
          todo->visible = true;
          visible_todos--;
        } else {
          todo->visible = false;
          continue;
        }

        /* Make checkbox red or green */
        if (todo->running) {
          clrk_draw_text(x, y, "[*] ", CLRK_COLOR_RUNNING_TRUE, CLRK_COLOR_TODO_BG);
        } else if (todo->checked) {
          clrk_draw_text(x, y, "[X] ", CLRK_COLOR_CHECKED_TRUE, CLRK_COLOR_TODO_BG);
        } else {
          clrk_draw_text(x, y, "[ ] ", CLRK_COLOR_CHECKED_FALSE, CLRK_COLOR_TODO_BG);
        }

        if (todo == clrk_list_data(project->current)) {
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
    "h: Move to previous project",
    "l: Move to next project",
    "j: Move to next todo",
    "k: Move to previous todo",
    "",
    "Action keys",
    "p: Create new project",
    "P: Delete current project",
    "E: Edit current project",
    "t: Create new todo",
    "T: Delete current todo",
    "e: Edit current todo",
    "Space: Toggle todo state",
    "",
    "ESC: Cancel input",
    "",
    "?: Help",
    "L: Load json config",
    "S: Save",
    "Q: Quit clerk",
    "",
    "Press any key to continue...",
    "END OF HELP"
  };

  /* Calculate number of lines */
  unsigned lines = 0;
  while (strcmp(help_text[i++], "END OF HELP")) {
    lines++;
  }
  LOG("Number of helper text lines %d", lines);

  if (lines >= height) {
    clrk_draw_status("Window too small to draw help box.");
    return;
  }
  /* Draw a box in the bottom of the screen */
  unsigned start_line = (height - 1) - lines;
  unsigned text_height = height - start_line;
  struct tb_cell cells[width * text_height];

  LOG("before defining background cells");
  for (i = 0; i < (width * text_height); ++i) {
    cells[i].ch = ' ';
    cells[i].fg = CLRK_COLOR_INPUT_FG;
    cells[i].bg = CLRK_COLOR_INPUT_BG;
  }

  LOG("before drawing box height: %d, start_line: %d, helpsize: %d", height, start_line, lines);
  tb_blit(0, start_line, width, text_height, cells);

  /* Draw help text */
  LOG("before drawing text");
  for (i = 0; i < lines; ++i) {
    clrk_draw_text(1, start_line + i, help_text[i], CLRK_COLOR_INPUT_FG, CLRK_COLOR_INPUT_BG);
  }

  LOG("END");
}

void clrk_draw(void) {
  clrk_draw_project_line();
  clrk_draw_todos();
}

