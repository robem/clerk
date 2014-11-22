#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <yajl/yajl_gen.h>    // gennerate json
#include <yajl/yajl_parse.h>  // react while parsing
#include <yajl/yajl_tree.h>   // work on tree after parsing

#include <clerk.h>
#include <clerk_draw.h>
#include <clerk_log.h>

extern clrk_clerk_t clerk;

static void clrk_json_write(void *ctx, const unsigned char *string, size_t len)
{
  HERE();
  fprintf((FILE*)ctx, "%s", string);
}

void clrk_save(void)
{
  HERE();
  FILE *file;
  struct yajl_gen_t *g;

  clrk_list_elem_t *project_elem, *todo_elem;
  clrk_project_t *project;
  clrk_todo_t *todo;

  clerk.json = clerk.json ? clerk.json : CLRK_CONFIG_FILE;
  file = fopen(clerk.json, "w");
  assert(file);

  /* Set print callback and pretty output */
  g = yajl_gen_alloc(NULL);
  yajl_gen_config(g, yajl_gen_print_callback, clrk_json_write, (void*)file);
  yajl_gen_config(g, yajl_gen_beautify);

  /* Start with '{' */
  yajl_gen_map_open(g);

  LIST_FOREACH(project_elem, clerk.project_list) {
    project = clrk_list_elem_data(project_elem);
    /*
     * Each project is represented as list of tuples.
     * <string> : [ {...}, {...}, ... ]
     */

    yajl_gen_string(g, (const unsigned char*)project->name, \
                    strlen(project->name));
    yajl_gen_array_open(g);

    LIST_FOREACH(todo_elem, project->todo_list) {
      todo = clrk_list_elem_data(todo_elem);
      /*
       * Each todo is represented as tuple
       * { CLRK_CONFIG_TEXT : <string>,
       *   CLRK_CONFIG_X    : <bool> }
       */

      yajl_gen_map_open(g);

      /* Text */
      yajl_gen_string(g, (const unsigned char*)CLRK_CONFIG_TEXT, \
                      strlen(CLRK_CONFIG_TEXT));
      yajl_gen_string(g, (const unsigned char*)todo->message, \
                      strlen(todo->message));
      /* X */
      yajl_gen_string(g, (const unsigned char*)CLRK_CONFIG_X, \
                      strlen(CLRK_CONFIG_X));
      yajl_gen_integer(g, todo->state);

      yajl_gen_map_close(g);
    }

    yajl_gen_array_close(g);
  }

  /* End with '}' */
  yajl_gen_map_close(g);

  fflush(file);
  fclose(file);
}

/*
 * Parser callback functions
 */

static int yajl_integer(void *ctx, long long i)
{
   HERE();
   switch (i) {
      case CHECKED:
         clrk_todo_tick_off();
         break;
      case RUNNING:
         clrk_todo_running();
         break;
      case INFO:
         clrk_todo_info();
         break;
   }
   return 1;
   LOG("END");
}

static int yajl_string(void *ctx, const unsigned char* string, size_t len)
{
  HERE();
  char *buf;

  /* Write todo text */
  buf = strndup((const char*)string, len);
  buf[len] = '\0';
  clrk_todo_add((const char*)buf);
  return 1;
}

static int yajl_map_key(void *ctx, const unsigned char* key, size_t len)
{
  HERE();
  clrk_config_tracker_t *cfg = ctx;
  char buf[100];
  strncpy(buf, (const char*)key, len);
  buf[len] = '\0';

  LOG("map key %s\n", buf);

  /* Create project */
  if (!cfg->in_todo) {
    clrk_project_add(buf);
  }
  return 1;
}

static int yajl_start_array(void *ctx)
{
  HERE();
  clrk_config_tracker_t *cfg = ctx;
  cfg->in_todo = true;
  return 1;
}

static int yajl_end_array(void *ctx)
{
  HERE();
  clrk_config_tracker_t *cfg = ctx;
  cfg->in_todo = false;
  return 1;
}

bool clrk_read_config(void)
{
  HERE();
  FILE *config;
  size_t file_size;
  char *buffer;
  yajl_val node;
  char err_buf[1024];
  const char **path_runner;
  unsigned i, num_of_options;

#define PATH_DEPTH 5
  const char *color_paths[][PATH_DEPTH] = {
    {"colors" ,"background"  ,(const char*) 0},
    {"colors" ,"project"     ,"foreground" ,(const char*) 0},
    {"colors" ,"project"     ,"background" ,(const char*) 0},
    {"colors" ,"project"     ,"selected"   ,(const char*) 0},
    {"colors" ,"todo"        ,"foreground" ,(const char*) 0},
    {"colors" ,"todo"        ,"background" ,(const char*) 0},
    {"colors" ,"todo"        ,"selected"   ,(const char*) 0},
    {"colors" ,"todo-state"  ,"todo"       ,(const char*) 0},
    {"colors" ,"todo-state"  ,"done"       ,(const char*) 0},
    {"colors" ,"todo-state"  ,"star"       ,(const char*) 0},
    {"colors" ,"todo-state"  ,"info"       ,(const char*) 0},
    {"colors" ,"status-line" ,"pormpt"     ,"foreground"    ,(const char*) 0},
    {"colors" ,"status-line" ,"prompt"     ,"background"    ,(const char*) 0},
    {"colors" ,"status-line" ,"input"      ,"foreground"    ,(const char*) 0},
    {"colors" ,"status-line" ,"input"      ,"background"    ,(const char*) 0}
  };

  /* XXX Order is important. Has to match the above. */
  int *color_options[] = {
    &clerk.colors->bg,
    &clerk.colors->project_fg,
    &clerk.colors->project_bg,
    &clerk.colors->project_selected,
    &clerk.colors->todo_fg,
    &clerk.colors->todo_bg,
    &clerk.colors->todo_selected,
    &clerk.colors->todo,
    &clerk.colors->done,
    &clerk.colors->star,
    &clerk.colors->info,
    &clerk.colors->todo_selected,
    &clerk.colors->prompt_fg,
    &clerk.colors->prompt_bg,
    &clerk.colors->input_fg,
    &clerk.colors->input_bg,
  };

  /* XXX assuming 64 bit */
  num_of_options = sizeof(color_paths) / (8 * PATH_DEPTH);
#undef PATH_DEPTH

  clerk.config = clerk.config ? clerk.config : CLRK_CONFIG_FILE;

  config = fopen(clerk.config, "r");
  if (config == NULL) {
    LOG("END");
    clrk_draw_status("Couldn't open config file: "CLRK_CONFIG_FILE);
    return false;
  }

  fseek(config, 0, SEEK_END);
  file_size = ftell(config);
  fseek(config, 0, SEEK_SET);

  buffer = (char*)malloc(file_size);
  fread(buffer, file_size, 1, config);
  fclose(config);

  node = yajl_tree_parse((const char*)buffer, err_buf, sizeof(err_buf));

  if (node == NULL) {
    LOG("END");
    clrk_draw_status("yajl_tree_parse failed on: "CLRK_CONFIG_FILE);
    goto out;
  }

  /* Get color options */
  for (i=0; i<num_of_options; ++i) {
    path_runner = color_paths[i];
    yajl_val v = yajl_tree_get(node, path_runner, yajl_t_number);

    if (v && YAJL_IS_INTEGER(v)) {
      *color_options[i] = YAJL_GET_INTEGER(v);
      LOG("Set color option to value %d", *color_options[i]);
    } else {
      /* Make sure it is _not_ 0 due 0 is a valid color value */
      LOG("Set color option to -1");
      *color_options[i] = -1;
    }
  }

  free(buffer);
  LOG("END");
  return true;

out:
  free(buffer);
  LOG("END");
  return false;
}

bool clrk_load(void)
{
  HERE();
  FILE *config;
  size_t file_size;
  char buffer[CLRK_CONFIG_BUFFER_SIZE];
  yajl_handle parser_handle;
  yajl_status parser_status;

  /* Free existing projects and todos  */
  while (clerk.current) {
    clrk_project_remove_current();
  }

  /* Load config file */
  clerk.json = clerk.json ? clerk.json : CLRK_DATA_FILE;
  LOG("before openfile %s", clerk.json);
  config = fopen(clerk.json, "r");

  LOG("before draw status");
  if (config == NULL) {
    LOG("END");
    return false;
  }

  LOG("before get file size");
  fseek(config, 0, SEEK_END);
  file_size = ftell(config);
  fseek(config, 0, SEEK_SET);

  if (file_size > CLRK_CONFIG_BUFFER_SIZE) {
    clrk_draw_status("File size greater than buffer size: "CLRK_DATA_FILE);
    goto out;
  }

  LOG("before read");
  fread(buffer, file_size, 1, config);

  LOG("before close");
  fclose(config);

  /* Prepare for parsing */
  yajl_callbacks callbacks = {
    .yajl_integer     = yajl_integer,
    .yajl_string      = yajl_string,
    .yajl_map_key     = yajl_map_key,
    .yajl_start_array = yajl_start_array,
    .yajl_end_array   = yajl_end_array,
  };

  clrk_config_tracker_t parser_context = {
    .in_todo = false,
  };

  LOG("before alloc");
  parser_handle = yajl_alloc((const yajl_callbacks*)&callbacks, NULL, &parser_context);

  LOG("before parse");
  parser_status = yajl_parse(parser_handle, (const unsigned char*)buffer, file_size);

  if (parser_status != yajl_status_ok) {
    yajl_free(parser_handle);
    LOG("%d", parser_status);
    LOG("End");
    return false;
  }

  LOG("before free");
  yajl_free(parser_handle);

  return true;
  LOG("END");
out:
  fclose(config);
  LOG("END");
  return false;
}

