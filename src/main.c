#include <getopt.h>
#include <stdio.h>
#include <clerk.h>

void usage(char *program_name)
{
  printf("Usage: %s\n\n"
         "  -c|--config\tSpecify config\n"
         "  -t|--todos\tSpecify todo data file\n"
         "  -h|--help\tPrint this screen\n",
         program_name);
}

int main(int argc, char *const *argv)
{
  LOG(BLUE"============================"NOCOLOR);

  int opt, option_index;
  const char *short_options = "ht:c:";
  const char *config = NULL;
  const char *todos = NULL;
  struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"config", required_argument, 0, 'c'},
    {"todos", required_argument, 0, 't'},
    {0, 0, 0, 0}
  };

  /* Gather some user wishes */
  while (1) {
    opt = getopt_long(argc, argv, short_options,
                      long_options, &option_index);

    if (opt == -1) {
      /* Parsing done */
      break;
    }

    switch (opt) {
      case 'h':
        usage(argv[0]);
        return EXIT_SUCCESS;
      case 'c':
        config = optarg;
        break;
      case 't':
        todos = optarg;
        break;
      case '?':
        return EXIT_FAILURE;
    }
  }

  tb_init();
  tb_clear();

  tb_select_output_mode(TB_OUTPUT_256);

  clrk_init(todos, config, &tb_shutdown);

  /*
   * Depending on whether the loop exited by means of tb_shutdown (as supplied
   * to clrk_init) we do not invoke here ourselves.
   */
  if (!clrk_loop_normal()) {
    tb_shutdown();
  }
  return EXIT_SUCCESS;
}
