#ifndef _CLRK_SIG_H
#define _CLRK_SIG_H

#include <stdbool.h>

typedef void (*clrk_exit_func)(void);

bool clrk_sig_init(clrk_exit_func exit_func);

#endif
