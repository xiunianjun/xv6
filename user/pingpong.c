#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char byte[1];
    byte[0] = 'c';
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    if(fork()==0){
	//接收ping的一个byte
	close(p1[1]);
	read(p1[0],byte,1);
	close(p1[0]);

	//构造输出字符串并输出
	int pid = getpid();
        char pid_str[ensure_itoa_capacity(pid)];
	itoa(pid,pid_str);	
	char meg_str[] = ": received ping\n";
	char buf[strlen(pid_str)+strlen(meg_str)+1];
	strcat(buf,pid_str,meg_str);		
	fprintf(1,buf);
	//fprintf(1,"%d: received ping\n",getpid());
	
	//子进程发送pong
	close(p2[0]);
	write(p2[1],byte,1);
	close(p2[1]);
	exit(0);
    }else {
	close(p1[0]);
	//p1的write端由父进程拿着
	write(p1[1],byte,1);
	close(p1[1]);
	//读取pong
	close(p2[1]);
	read(p2[0],byte,1);
	close(p2[0]);
	//构造
	int pid = getpid();
        char pid_str[ensure_itoa_capacity(pid)];
	itoa(pid,pid_str);	
	char meg_str[] = ": received pong\n";
	char buf[strlen(pid_str)+strlen(meg_str)+1];
	strcat(buf,pid_str,meg_str);		
	fprintf(1,buf);
	//fprintf(1,"%d: received pong\n",getpid());
	exit(0);
    }
    exit(0);
}
