#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"
#include "sysinfo.h"

int 
sysinfo(struct sysinfo* info){
  struct sysinfo res;
  res.nproc = countproc();
  res.freemem = countfree();
  struct proc *p = myproc();
  if(copyout(p->pagetable, (uint64)info,(char *)(&res), sizeof(res)) != 0)
    return -1;
  return 1;
}
