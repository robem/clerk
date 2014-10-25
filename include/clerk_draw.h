#ifndef _CLERK_DRAW_H
#define _CLERK_DRAW_H

// Coordinates
#define CLRK_DRAW_PRJ_LINE_Y   0
#define CLRK_DRAW_TODO_START_X 3
#define CLRK_DRAW_TODO_START_Y (CLRK_DRAW_PRJ_LINE_Y+3)
#define CLRK_DRAW_STATUS_LINE (tb_height()-1)

#define CLRK_DRAW_BOX_WIDTH  100
#define CLRK_DRAW_BOX_HEIGHT 10

#define CLRK_DRAW_NUM_TODO_STATES 10

// Colors
#define CLRK_COLOR_PRJ_LINE       17
#define CLRK_COLOR_PRJ_BG         17
#define CLRK_COLOR_PRJ_FG         15
#define CLRK_COLOR_PRJ_CURRENT    24 

#define CLRK_COLOR_TODO_BG       232
#define CLRK_COLOR_TODO_FG       192
#define CLRK_COLOR_RUNNING_TRUE  208
#define CLRK_COLOR_INFO_TRUE     226
#define CLRK_COLOR_CHECKED_TRUE   40
#define CLRK_COLOR_CHECKED_FALSE 160
#define CLRK_COLOR_TODO_CURRENT  239

#define CLRK_COLOR_PROMPT_FG 15
#define CLRK_COLOR_PROMPT_BG 160

#define CLRK_COLOR_INPUT_FG  15
#define CLRK_COLOR_INPUT_BG 235

struct state_descriptor {
  char c;
  unsigned color;
};

struct state_descriptor state_description[CLRK_DRAW_NUM_TODO_STATES];

/*
 * Draw text from left to right starting from position(x, y).
 *
 * Returns number of text's characters.
 */
unsigned int clrk_draw_text(int x, int y, const char* text, int foreground, int background);

/*
 * Clearing a line with space characters and one background color.
 */
int clrk_draw_line(int line, int background);

/*
 * Draw Clerks header.
 */
void clrk_draw_project_line(void);

/*
 * Draw todo area
 */
void clrk_draw_todos(void);

/*
 * Draw command line
 */
void clrk_draw_show_input_line(void);

/*
 * Draw command line
 */
void clrk_draw_remove_input_line(void);

/*
 * Draw status in command line
 */
void clrk_draw_status(const char *status);

/*
 * Draw help text
 */
void clrk_draw_help(void);

/*
 * Initialize todo states
 */
void clrk_draw_init(void);

/*
 * Draw projects and todos
 */
void clrk_draw(void);

#endif
