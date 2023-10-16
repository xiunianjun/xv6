#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

uint64 min(uint64 a,uint64 b){return a>b?b:a;}

int munmap(void* address,size_t length){
    struct proc* p = myproc();
    uint64 va = (uint64)address;
    for(int i=0;i<NFILEMAP;i++){
      if(p->filemaps[i].isused&&p->filemaps[i].va<=va&&p->filemaps[i].va+length>va){
        uint64 start_va;
        if(va == p->filemaps[i].va)
                start_va = PGROUNDUP(p->filemaps[i].va);
        else
                start_va = PGROUNDDOWN(va);
        uint64 bounder = p->filemaps[i].va + min(p->filemaps[i].file->ip->size,length);
        //在这里需要用off进行读写，所以需要对原本的加载处off手动保存
        uint64 tmp_off = p->filemaps[i].file->off;
        p->filemaps[i].file->off = p->filemaps[i].offset+va-p->filemaps[i].va;

        //释放已经申请的页表项、内存，并且看看是不是需要写回
        while(start_va < bounder){
          if(p->filemaps[i].flags == MAP_SHARED){
            //写回
            filewrite(p->filemaps[i].file,start_va,PGSIZE);
          }
          uvmunmap(p->pagetable,start_va,1,1);
          start_va += PGSIZE;
        }

        //修改filemap结构体的起始地址va和长度，offset也要变，因为他记录va对应的是文件哪个位置
        if(va == p->filemaps[i].va){
          //释放的是头几页
          p->filemaps[i].offset += length;
          p->filemaps[i].va = va+length;
          p->filemaps[i].length -= length;
        }else {
          //释放的是尾几页
          p->filemaps[i].length -= p->filemaps[i].length - va;
        }
        // 检验map的合理性
        if(p->filemaps[i].length == 0 || p->filemaps[i].va >= p->filemaps[i].va+length
                        || p->filemaps[i].file->off > p->filemaps[i].file->ip->size){
          p->filemaps[i].isused = 0;//释放
          fileclose(p->filemaps[i].file);
        }
        p->filemaps[i].file->off = tmp_off;
      }
    }
    return 0;
}

#define ERRORADDR 0xffffffffffffffff

// 映射file从offset开始长度为length的内容到内存中，返回内存中的文件内容起始地址
void* mmap(void* address,size_t length,int prot,int flags,struct file* file,uint64 offset){
    // mmap的prot权限必须与file的权限对应，不能file只读但是mmap却可写且shared
    if((prot&PROT_WRITE) != 0&&flags == MAP_SHARED &&file->writable == 0)       
        return (void*)ERRORADDR;
    
    struct proc* p = myproc();
    uint64 va = 0;
    int i=0;
    
    //找到filemap池中第一个空闲的filemap
    for(i=0;i<NFILEMAP;i++){
      if(!p->filemaps[i].isused){
        // 获取va,也即真正的address
        va = p->sz;
        p->sz += length;
        // 其实这里用一个memcpy会更加优雅，可惜我忘记了（）
        p->filemaps[i].isused = 1;
        p->filemaps[i].va = va;
        p->filemaps[i].okva = va;
        p->filemaps[i].length = length;
        p->filemaps[i].prot = prot;
        p->filemaps[i].flags = flags;
        p->filemaps[i].file = file;
        p->filemaps[i].file->off = offset;
        p->filemaps[i].offset = offset;
        // 增加文件引用数
        filedup(file);
        break;
      }
    }
    if(va == 0)  return (void*)ERRORADDR;
    // return start of free memory
    uint64 start_va = PGROUNDUP(va);
    // 先读入处于proc已申请的内存页区域（也即没有内存对齐情况下）
    uint64 off = start_va - va;
    if(off < PGSIZE){
        fileread(file,va,off);
        file->off += off;
        p->filemaps[i].okva = va+off;
    }
    return (void*)va;
}
