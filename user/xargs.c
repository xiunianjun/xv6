#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
int
main(int argc, char *argv[])
{
   // for(int i=0;i<argc;i++)	fprintf(1,"argv[%d] = %s\n",i,argv[i]);
    //之所以写得这么曲折，是因为数组是const指针不能变值
    char buffer[512];
    char* buf = buffer; 
    char* start = buf;
    //如果xargs没有参数，就开启复读模式
    if(argc <= 1){
		int n;
		while((n = read(0,buf,512))){
	    	write(1,buf,n);
		}
		exit(0);
    }

    //这个必须写在外面，不能定义在下面的if体里，不然出了作用域就会被回收，不能作为参数的一员参与进exec，会变成很可怕的乱码。
    char tmp[512];
    //MAXARG为定义在kernel/param.h下的参数，代表参数最多值
    char* new_argv[MAXARG] = {0};
    argc--;
    int i = 0;
    for(i=0; i<argc; i++){
		new_argv[i] = argv[i+1];
    }
    while(read(0,buf,1)){
        //fprintf(1,"buf[0] is : %c\n",buf[0]);
        if(buf[0] == '\n'){
            memcpy(tmp,start,buf-start);
            tmp[buf-start] = '\0';
            new_argv[i++] = tmp;
            argc++;
            buf = start;
           // fprintf(1,"tmp is :%s\n",tmp);
            //fprintf(1,"new_argv[i-1] is :%s\n",new_argv[i-1]);
        } else{
            buf ++;
        }
    }
   // fprintf(1,"argc = %d\n",argc);
    if(fork() == 0){
    	exec(new_argv[0],new_argv);
    } else{
		wait(0);
    }
    exit(0);
}
