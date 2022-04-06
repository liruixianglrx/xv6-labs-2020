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

struct hashbucket{
  struct buf head;
  struct spinlock lock;
};
struct {
  struct buf buf[NBUF];
  struct hashbucket bucket[13];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

} bcache;


void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  /*bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;*/
  
  for (int i=0;i<13;i++){            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  initlock(&bcache.bucket[i].lock, "bcache");
  bcache.bucket[i].head.prev=&bcache.bucket[i].head;
  bcache.bucket[i].head.next=&bcache.bucket[i].head;
  }
  
  
  /*for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
    }*/
    
   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.bucket[0].head.next;
    b->prev = &bcache.bucket[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].head.next->prev = b;
    bcache.bucket[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  //acquire(&bcache.lock);
  int bno=blockno % 13;
  acquire (&bcache.bucket[bno].lock);

  // Is the block already cached?
  /*for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }*/
    
    
    for(b = bcache.bucket[bno].head.next; b != &bcache.bucket[bno].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      
      //record ticks
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);
      
      release(&bcache.bucket[bno].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  /*for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }*/
    
    struct buf* tmp;
    b=0;
    
    for (int i=bno,times=0;times<13;i=(i+1)%13){
    ++times;
    if (i!=bno)       //we have previously acquired bcache.bucket[bno].lock
    {                 //dont acquire it again
    if (!holding(&bcache.bucket[i].lock))
       acquire (&bcache.bucket[i].lock);
       else continue; //if lock is being held,try to acquire next lock
    }
    
    for (tmp=bcache.bucket[i].head.prev; tmp != &bcache.bucket[i].head; tmp = tmp->prev){
    if (tmp->refcnt==0 && (b==0 || tmp->timestamp < b->timestamp ))
        b=tmp;//using LRU strategy to find the buffer
    }
    
    if (b)
    {
    if (i != bno){
     b->next->prev=b->prev;
     b->prev->next=b->next;
     release(&bcache.bucket[i].lock);
     
     b->next=bcache.bucket[bno].head.next;
     b->prev=&bcache.bucket[bno].head;
     bcache.bucket[bno].head.next=b;
     b->next->prev=b;
    }
     
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);
      
      
      release(&bcache.bucket[bno].lock);
      acquiresleep(&b->lock);
      return b;   
    }else {
    if (i!=bno)
       release(&bcache.bucket[i].lock);
    }
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
  int bno=b->blockno %13;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bucket[bno].lock);
  b->refcnt--;
  
    // no one is waiting for it.
   acquire(&tickslock);
   b->timestamp = ticks;
   release(&tickslock);

  
  release(&bcache.bucket[bno].lock);
}

void
bpin(struct buf *b) {
  int bno=b->blockno %13;
  acquire(&bcache.bucket[bno].lock);
  b->refcnt++;
  release(&bcache.bucket[bno].lock);
}

void
bunpin(struct buf *b) {
  int bno=b->blockno %13;
  acquire(&bcache.bucket[bno].lock);
  b->refcnt--;
  release(&bcache.bucket[bno].lock);
}


