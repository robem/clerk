#include <assert.h>
#include <signal.h>
#include <stddef.h>

#include <clerk_log.h>
#include <clerk_json.h>
#include <clerk_sig.h>

/* A signal that triggers a clrk_save(). */
#define CLRK_SIGNAL_SAVE SIGUSR1

typedef struct sigaction sigaction_t;

static void clrk_save_handler(int sig, siginfo_t *siginfo, void *context)
{
  HERE();
  assert(sig == CLRK_SIGNAL_SAVE);

  clrk_save();
  LOG("END");
}

bool clrk_sig_init(void)
{
  int result;
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

  result = sigaction(CLRK_SIGNAL_SAVE, &sig, NULL);
  return result == 0;
}
