#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mmap(void){
  uint64 addr;
  struct vma *v = 0;
  int len,prot,flag,fd;
  struct file *f;
  if((argaddr(0,&addr)<0)||(argint(1,&len)<0)||(argint(2,&prot)<0)||(argint(3,&flag)<0)||(argfd(4, &fd, &f) < 0)){
    return -1;
  }
  if((flag & MAP_SHARED) && (prot & PROT_WRITE) && (f->writable == 0)) {
      return -1;
  }
  struct proc *p;
  p=myproc();
  for(int i = 0; i < 16; i++) {
      if(!p->VMAS[i].used) {
          v = &p->VMAS[i];
          break;
      }
  }
  if(v==0) return -1;
  uint64  newmaphigh=PGROUNDDOWN(p->maphigh - len);
  if(newmaphigh<p->sz)
  {
    printf("maphigh is lower than heaptop!\n");
    return -1;
  }
  else{
    p->maphigh=newmaphigh;
  }
  v->va_start=p->maphigh;
  v->len=len;
  v->used=1;
  v->prot=prot;
  v->flags=flag;
  v->opfile=f;
  
  filedup(f);
  return v->va_start;
}
uint64
sys_munmap(void){
  uint64 addr;
  int len;
  struct file* f;
  struct vma *v = 0;
  
  if((argaddr(0,&addr)<0)||(argint(1,&len)<0)){
    return -1;
  }
  struct proc *p;
  p=myproc();
  for(int i = 0; i < 16; i++) {
    if(p->VMAS[i].used && addr >= p->VMAS[i].va_start && addr < p->VMAS[i].va_start + p->VMAS[i].len) {
        v = &p->VMAS[i];
        break;
    }
  }
  if(v==0) return -1;

  if(v->flags & MAP_SHARED&& (v->prot & PROT_WRITE)){
    uint64 cur=addr;
    f=v->opfile;
    while(cur<addr+len){
      pte_t* pte=walk(p->pagetable,cur,0);
      if(pte&&(*pte&PTE_V)&&(*pte&PTE_D)){
        uint64 file_off=cur-v->va_start;
        begin_op();
        ilock(f->ip);
        writei(f->ip, 1, cur, file_off, PGSIZE);
        iunlock(f->ip);
        end_op();
        *pte &= ~PTE_D;
      }
      cur+=PGSIZE;
    }
    
  }
  int npg=len / PGSIZE;
  uvmunmap(p->pagetable,addr,npg,1);
  if(addr==v->va_start&&len==v->len){
    fileclose(v->opfile);
    v->used=0;
  }
  else if(addr==v->va_start){
    v->va_start+=len;
    v->len-=len;
  }
  else{
    v->len-=len;
  }
  return 0;
}