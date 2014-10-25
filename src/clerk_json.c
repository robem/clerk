#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <yajl/yajl_gen.h>    // gennerate json
#include <yajl/yajl_parse.h>  // parse json

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
  clerk.json = clerk.json ? clerk.json : CLRK_CONFIG_FILE;
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
    clrk_draw_status("File size greater than buffer size: "CLRK_CONFIG_FILE);
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
  parser_status = yajl_parse(parser_handle, (const unsigned char*)buffer, sizeof(buffer));

  LOG("before free");
  yajl_free(parser_handle);

  return true;
  LOG("END");
out:
  fclose(config);
  LOG("END");
  return false;
}

