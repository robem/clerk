#include <clerk.h>

int main(int argc, const char *argv[])
{
  LOG(BLUE"============================"NOCOLOR);

  tb_init();
  tb_clear();

  tb_select_output_mode(TB_OUTPUT_256);

  clrk_init();

  clrk_loop_normal();

  tb_shutdown();
  return 0;
}
