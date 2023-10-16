#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

int
trace(int mask){
    struct proc *p = myproc();
    p->mask = mask;
    return 1;    
}

int
istraced(int callid){
    struct proc *p = myproc();
    if(((p->mask >> callid) & 1) == 1){
	return 1;
    } else{
    	return 0;
    }
}
