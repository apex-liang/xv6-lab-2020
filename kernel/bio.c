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
struct buf buf[NBUF];
struct {
  struct spinlock lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache[NBUCKET];

void
binit(void)
{
  struct buf *b;
  int idx=0;
  // initlock(&bcache.lock, "bcache");
  for(int i=0;i<NBUCKET;i++)
  {
    initlock(&bcache[i].lock, "bcache");
    bcache[i].head.prev = &bcache[i].head;
    bcache[i].head.next = &bcache[i].head;
  }
  // Create linked list of buffers
  b=buf;
  for(idx = 0; idx < NBUF; idx++){
    int id=idx%NBUCKET;
    b->next = bcache[id].head.next;
    b->prev = &bcache[id].head;
    b->releasetime = 0;
    b->bucketid=id;
    initsleeplock(&b->lock, "buffer");
    bcache[id].head.next->prev = b;
    bcache[id].head.next = b;
    b++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id=blockno%NBUCKET;
  acquire(&bcache[id].lock);

  // Is the block already cached?
  for(b = bcache[id].head.next; b != &bcache[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache[id].head.prev; b != &bcache[id].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache[id].lock);//it is ok to release because all buf in bcache[id] is used.None of other could stolen buf in this bucket.
  struct buf *bLRU=0;
  int idLRU=0;
  uint minticks=0xffffffff;
  for(int i=0;i<NBUCKET;i++)
  {
    if(i==id) continue;
    acquire(&bcache[i].lock);

    for(b = bcache[i].head.prev; b != &bcache[i].head; b = b->prev){
      if(b->refcnt == 0) {
        if(minticks>b->releasetime)
        {
          minticks=b->releasetime;
          bLRU=b;
          idLRU=i;
          break;
        }
      }
    }
    release(&bcache[i].lock);
  }
  struct buf *tmp;
  if(bLRU)
  {
    if(idLRU<id)
    {
      acquire(&bcache[idLRU].lock);
      acquire(&bcache[id].lock);
    }
    else  
    {
      acquire(&bcache[id].lock);
      acquire(&bcache[idLRU].lock);
    }
    for(tmp = bcache[id].head.next; tmp != &bcache[id].head; tmp = tmp->next){
      if(tmp->dev == dev && tmp->blockno == blockno){
        tmp->refcnt++;
        release(&bcache[id].lock);
        release(&bcache[idLRU].lock);
        acquiresleep(&tmp->lock);
        return tmp;
      }
    }
    if(bLRU->refcnt!=0)
    {
      if(idLRU<id)
      {
        release(&bcache[idLRU].lock);
        release(&bcache[id].lock);
      }
      else  
      {
        release(&bcache[id].lock);
        release(&bcache[idLRU].lock);
      }
      return bget(dev,blockno);
    }
    
    bLRU->dev = dev;
    bLRU->blockno = blockno;
    bLRU->valid = 0;
    bLRU->refcnt = 1;
    bLRU->bucketid=id;
    bLRU->next->prev = bLRU->prev;
    bLRU->prev->next = bLRU->next;
    bLRU->next = bcache[id].head.next;
    bLRU->prev = &bcache[id].head;
    bcache[id].head.next->prev = bLRU;
    bcache[id].head.next = bLRU;
    if(idLRU<id)
    {
      release(&bcache[idLRU].lock);
      release(&bcache[id].lock);
    }
    else  
    {
      release(&bcache[id].lock);
      release(&bcache[idLRU].lock);
    }
    acquiresleep(&bLRU->lock);
    // printf("OK!bLRU->bucketid:%d dev:%d blockno:%d \n",bLRU->bucketid,dev,blockno);
    return bLRU;
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id=b->bucketid;

  acquire(&bcache[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache[id].head.next;
    b->prev = &bcache[id].head;
    b->releasetime =ticks;
    bcache[id].head.next->prev = b;
    bcache[id].head.next = b;
  }
  
  release(&bcache[id].lock);
}

void
bpin(struct buf *b) {
  int id=b->bucketid;
  acquire(&bcache[id].lock);
  b->refcnt++;
  release(&bcache[id].lock);
}

void
bunpin(struct buf *b) {
  int id=b->bucketid;
  acquire(&bcache[id].lock);
  b->refcnt--;
  release(&bcache[id].lock);
}


