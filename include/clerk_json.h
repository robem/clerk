#ifndef _CLRK_JSON_H
#define _CLRK_JSON_H

#define CLRK_CONFIG_BUFFER_SIZE 1024*1024

/*
 * Keep information while parsing
 * to know where we are
 */
typedef struct clrk_config_tracker {
  bool in_todo;
  bool marked;
  bool text;
} clrk_config_tracker_t;

/*
 * Writes all projects and their todos into
 * a JSON file.
 */
void clrk_save(void);

/*
 * Loads JSON config file
 */
void clrk_load(void);

#endif
