#include "kernel/types.h"
#include "user/user.h"

int main(){
	int p[2];
	pipe(p);
    
	if(fork() == 0){
		while(1){
			char buf[3];
			//读入第一个数字 
			read(p[0],buf,3);
			int prime = atoi(buf);
			if(prime == 36){
				close(p[0]);
				close(p[1]);
				exit(0);
			}
			fprintf(1,"prime %d\n",prime);
			//读入其他数字 
			int tmp = atoi(buf);
			while(1){
				read(p[0],buf,3);
				tmp = atoi(buf);
                //输入结束
				if(tmp == 36){
					break;
				}
				if(tmp%prime!=0){
					write(p[1],buf,3);
				}
			}
            //作为标记，标志着输入序列结束
			itoa(36,buf);
			write(p[1],buf,3);
			if(fork()){
			}
			else{
				close(p[0]);
				close(p[1]);
				wait(0);
				exit(0);
			}
		}
	} else{
		close(p[0]);
		char buf[3];
		for(int i=2;i<=35;i++){
			itoa(i,buf);
			write(p[1],buf,3);
		}
        //作为标记，标志着输入序列结束
		itoa(36,buf);
		write(p[1],buf,3);
		close(p[1]);
		wait(0);
	}
	exit(0);
}
