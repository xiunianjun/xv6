#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"

int
main(int argc, char *argv[])
{
  int i;

  //argc应该指的是arg count，包含了arg[0]（命令名称）
  //如果命令为“sleep”，那么就输出“sleep: missing operand”
  if(argc <= 1){
    fprintf(2, "sleep:missing operand\n");
    exit(1);
  }

  for(i = 1; i < argc; i++){
    if(sleep(atoi(argv[i]))<0){
	fprintf(2,"sleep:there has something wromng.Stop running.\n");
	exit(1);
    }
  }
  exit(0);
}
