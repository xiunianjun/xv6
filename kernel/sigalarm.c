#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

int 
sigalarm(int intervel,void* handler)
{
   
    struct proc* p = myproc();
    p->interval = intervel;
    p->handler = handler;
    acquire(&tickslock);
    p->lasttick = ticks;
    release(&tickslock);
    if(handler == 0 && intervel == 0){
		return 0;
    }
    memmove(p->savetrap,p->trapframe,sizeof(struct trapframe));
    return 1;    
}

int
sigreturn()
{
    struct proc* p = myproc();
    memmove(p->trapframe,p->savetrap,sizeof(struct trapframe));
    p->flag = 0;
    return 0;
}