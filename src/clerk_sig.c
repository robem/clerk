#include <assert.h>
#include <signal.h>
#include <stddef.h>

#include <clerk_log.h>
#include <clerk_json.h>
#include <clerk_sig.h>

/* A signal that triggers a clrk_save(). */
#define CLRK_SIGNAL_SAVE SIGUSR1
#define CLRK_SIGNAL_EXIT1 SIGTERM
#define CLRK_SIGNAL_EXIT2 SIGINT

typedef struct sigaction sigaction_t;

static clrk_exit_func clrk_exit;

static void clrk_save_handler(int sig, siginfo_t *siginfo, void *context)
{
  HERE();
  assert(sig == CLRK_SIGNAL_SAVE);

  clrk_save();
  LOG("END");
}

static void clrk_exit_handler(int sig, siginfo_t *siginfo, void *context)
{
  HERE();
  assert(sig == CLRK_SIGNAL_EXIT1 || sig == CLRK_SIGNAL_EXIT2);
  assert(clrk_exit != NULL);

  (*clrk_exit)();
  LOG("END");
}

bool clrk_setup_save_handler(void)
{
  sigset_t mask;
  sigaction_t sig = {};

  sigemptyset(&mask);
  /*
   * We do not want to receive signals while handling one in order to not run
   * into concurrency issues.
   */
  sigaddset(&mask, CLRK_SIGNAL_SAVE);

  sig.sa_mask = mask;
  sig.sa_flags = SA_SIGINFO;
  sig.sa_sigaction = &clrk_save_handler;

  return sigaction(CLRK_SIGNAL_SAVE, &sig, NULL) == 0;
}

bool clrk_setup_exit_handler(clrk_exit_func exit_func)
{
  sigset_t mask;
  sigaction_t sig = {};

  assert(clrk_exit == NULL);

  sigemptyset(&mask);
  sigaddset(&mask, CLRK_SIGNAL_EXIT1);
  sigaddset(&mask, CLRK_SIGNAL_EXIT2);

  clrk_exit = exit_func;

  sig.sa_mask = mask;
  sig.sa_flags = SA_SIGINFO;
  sig.sa_sigaction = &clrk_exit_handler;

  return sigaction(CLRK_SIGNAL_EXIT1, &sig, NULL) == 0 &&
         sigaction(CLRK_SIGNAL_EXIT2, &sig, NULL) == 0;
}

bool clrk_sig_init(clrk_exit_func exit_func)
{
  return clrk_setup_save_handler() && clrk_setup_exit_handler(exit_func);
}
