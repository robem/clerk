#ifndef _CLERK_LOG_H
#define _CLERK_LOG_H

#if DEBUG
#include <stdio.h>

#define   NOCOLOR   "\033[0m"
#define   RED       "\033[31m"
#define   GREEN     "\033[32m"
#define   YELLOW    "\033[33m"
#define   BLUE      "\033[34m"
#define   MAGENTA   "\033[35m"
#define   CYAN      "\033[36m"

#define LOG(...) {\
  FILE *f = fopen("./LOG", "a"); \
  fprintf(f, "%s: ", __PRETTY_FUNCTION__); \
  fprintf(f, __VA_ARGS__); \
  fprintf(f, "\n"); \
  fflush(f); \
  fclose(f); \
}

#define HERE() {\
  FILE *f = fopen("./LOG", "a"); \
  fprintf(f, MAGENTA"%s\n"NOCOLOR, __PRETTY_FUNCTION__); \
  fflush(f); \
  fclose(f); \
}

#define PTR CYAN"%p"NOCOLOR

#else
#define   NOCOLOR
#define   RED
#define   GREEN
#define   YELLOW
#define   BLUE
#define   MAGENTA
#define   CYAN

#define LOG(...)
#define HERE()

#endif

#endif
