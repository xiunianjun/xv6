// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf dev_heads[NBUCKET]; // 每个元素都是一个双向链表
  struct spinlock dev_locks[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b = bcache.buf;

  // 一开始先均分
  uint64 tmp = NBUF / NBUCKET;
  uint64 t = tmp;
  for (int i = 0; i < NBUCKET; i++) {
    char buf[10];
    snprintf(buf, 9, "bcache%02d", i);
    initlock(&(bcache.dev_locks[i]), buf);

    bcache.dev_heads[i].prev = &(bcache.dev_heads[i]);
    bcache.dev_heads[i].next = &(bcache.dev_heads[i]);

    if(i == NBUCKET - 1){ // 最后一个
      for( ; b < bcache.buf + NBUF; b++) {
        b->timestamp = ticks;
        b->next = bcache.dev_heads[i].next;
        b->prev = &bcache.dev_heads[i];
        initsleeplock(&b->lock, "buffer");
        bcache.dev_heads[i].next->prev = b;
        bcache.dev_heads[i].next = b;
      }
    }else{ // 均分所有buffer块
      for(;b < bcache.buf + t; b++) {
        b->timestamp = ticks;
        // 把b加入到链表中
        b->next = bcache.dev_heads[i].next;
        b->prev = &bcache.dev_heads[i];
        initsleeplock(&b->lock, "buffer");
        bcache.dev_heads[i].next->prev = b;
        bcache.dev_heads[i].next = b;
      }
    }
    t += tmp;
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
//  printf("bget\n");
  uint hash = blockno % 13;

  acquire(&(bcache.dev_locks[hash]));

  // Is the block already cached?
  for(struct buf* b = bcache.dev_heads[hash].next; b != &(bcache.dev_heads[hash]); b = b->next){
    if(b->blockno == blockno&&b->dev == dev){ // 找到了
      b->refcnt++;
      b->timestamp = ticks;
     // printf("r2\n");
      release(&(bcache.dev_locks[hash]));
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 没找到，进行LRU
  // 遍历hash table，找到LRU，也即时间戳最小的且refcnt小于0的那一项

  acquire(&bcache.lock);

  uint min_time = 4294967295;// uint的最大值。此处不能使用(uint)(-1)
  // int target_id = hash;
  struct buf* goal = 0;
  for(int i = 0; i < NBUCKET; i++) {
    // int initial_hold = holding(&(bcache.dev_locks[i]));

    uint time = 0;
    // if (!initial_hold)
    //   acquire(&(bcache.dev_locks[i]));
    for(struct buf* b = bcache.dev_heads[i].prev; b != &(bcache.dev_heads[i]); b = b->prev){
      if(b->refcnt == 0) {
          time = b->timestamp;
          if(time < min_time){
            min_time = time;
            // if(goal && target_id != hash && target_id != i) {
            //  // printf("r1\n");
            //   release(&(bcache.dev_locks[target_id]));
            // }
            goal = b;
            // target_id = i;
          }
      }
    }
    // if (!initial_hold && target_id != i) {
    //          // printf("r3\n");
    //   release(&(bcache.dev_locks[i]));
    // }
  }
  // hashtable中存在着空闲buf
  if(goal != 0){
      goal->dev = dev;
      goal->blockno = blockno;
      goal->valid = 0;
      goal->refcnt = 1;

      // 将goal从其所在双向链表中移除

      goal->prev->next = goal->next;
      goal->next->prev = goal->prev;

      // 在新双向链表中添加goal
      goal->prev = &(bcache.dev_heads[hash]);
      goal->next = bcache.dev_heads[hash].next;

      bcache.dev_heads[hash].next->prev = goal;
      bcache.dev_heads[hash].next = goal;

      release(&(bcache.dev_locks[hash]));
      release(&bcache.lock);
      // if (target_id != hash)
      //   release(&(bcache.dev_locks[target_id]));
      acquiresleep(&goal->lock);

      return goal;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
//  printf("brelse\n");
  if(!holdingsleep(&b->lock))
    panic("brelse");

  uint hash = b->blockno%NBUCKET;

  acquire(&(bcache.dev_locks[hash]));
  b->refcnt--;
  b->timestamp = ticks;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    
    b->next = bcache.dev_heads[hash].next;
    b->prev = &bcache.dev_heads[hash];
    bcache.dev_heads[hash].next->prev = b;
    bcache.dev_heads[hash].next = b;
  }
  release(&(bcache.dev_locks[hash]));
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  uint hash = b->blockno%NBUCKET;
  acquire(&bcache.dev_locks[hash]);
  b->refcnt++;
  release(&bcache.dev_locks[hash]);
}

void
bunpin(struct buf *b) {
  uint hash = b->blockno%NBUCKET;
  acquire(&bcache.dev_locks[hash]);
  b->refcnt--;
  release(&bcache.dev_locks[hash]);
}
