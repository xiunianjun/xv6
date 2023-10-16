// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock kmem_locks[NCPU];
  struct run *freelists[NCPU];
} kmem;

void
kinit()
{
  for(int i=0;i<NCPU;i++){
    char buf[8];
    snprintf(buf,6,"kmem%d",i);
    initlock(&kmem.kmem_locks[i], buf);
  }
  freerange(end, (void*)PHYSTOP);
}

// 多带一个参数表示cpuid，仅在kinit的freerange中使用
void
kfree_init(void *pa,int i)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  r->next = kmem.freelists[i];
  kmem.freelists[i] = r;
}
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
   
  // 把空闲内存页均分给每个CPU
  uint64 sz = ((uint64)pa_end - (uint64)pa_start)/NCPU;
  uint64 tmp = PGROUNDDOWN(sz) + (uint64)p;
  for(int i=0;i<NCPU;i++){
    for(; p + PGSIZE <= (char*)tmp; p += PGSIZE)
      kfree_init(p,i);
    tmp += PGROUNDDOWN(sz);
    if(i == NCPU-2){
      tmp = (uint64)pa_end;
    }
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // 在这
  push_off();
  int id = cpuid();

  acquire(&kmem.kmem_locks[id]);
  r->next = kmem.freelists[id];
  kmem.freelists[id] = r;
  release(&kmem.kmem_locks[id]);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();

  acquire(&kmem.kmem_locks[id]);
  r = kmem.freelists[id];
  if(r){
    kmem.freelists[id] = r->next;
  }
  release(&kmem.kmem_locks[id]);
  pop_off();

  // 如果无空闲页，则窃取
  if(!r){
    for(int i=NCPU-1;i>=0;i--){
      acquire(&kmem.kmem_locks[i]);
      r = kmem.freelists[i];
      if(r){
        kmem.freelists[i] = r->next;
        release(&kmem.kmem_locks[i]);
        break;
      }
      release(&kmem.kmem_locks[i]);
    }
  }
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
