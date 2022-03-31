# LAB 8
## Memory allocator
本次实验是希望为每个cpu维护一个kmem，导致不会造成对同一个kmem锁的竞争
初始时为每一个锁初始化并且命名为“kmem”，本次实验的关键是在一个cpu的kmem里freelist耗尽时，窃取另一个cpu的freelis

在`kmem`中，将其定义为一个数组
```c
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];
```

在`kinit`中，为所有锁初始化以“kmem”开头的名称
```c
void
kinit()
{
  /*initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);*/
  for (int i=0;i<NCPU;i++)
  initlock(&kmem[i].lock,"kmem");
  freerange(end, (void*)PHYSTOP);
}
```

修改`kfree`，为每个cpu释放内存，注意push_off()的位置，
```c
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int pid=cpuid();
  acquire(&kmem[pid].lock);
  r->next = kmem[pid].freelist;
  kmem[pid].freelist = r;
  release(&kmem[pid].lock);
  pop_off();
}

```

修改`kalloc()`,目的是为了在一个cpu的freelist耗尽时偷取其他cpu的freelist，有以下两种的实现方式，个人认为都有其不足，下面会分别对两者进行分析

### 代码1
```c
void *
kalloc(void)
{
  struct run *r;

  push_off();// 关中断
  int id = cpuid();
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r)
    kmem[id].freelist = r->next;
  else {
    int antid;  // another id
    // 遍历所有CPU的空闲列表
    for(antid = 0; antid < NCPU; ++antid) {
      if(antid == id)
        continue;
      acquire(&kmem[antid].lock);
      r = kmem[antid].freelist;
      if(r) {
        kmem[antid].freelist = r->next;
        release(&kmem[antid].lock);
        break;
      }
      release(&kmem[antid].lock);
    }
  }
  release(&kmem[id].lock);
  pop_off();  //开中断

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
```

此代码先获取其他空闲cpu的lock，然后将其分配给耗尽的cpu，但是本人认为会在特点情况导致死锁
例如cpu1此时耗尽，然后acquire kmem[1]的锁，遍历至cpu2时，acquire kmem[2]的锁
假如此时cpu2已经持有kmem[2]的锁，并且freelist耗尽，此时cpu[1]会因为cpu2已经持有kmem[2]锁，在acquire kmem[2]时中一直spin，然后cpu2因为freelist耗尽，开始遍历cpu，此时又尝试获取kmem[1]的锁，又因为cpu1已经持有这个锁，故会造成死锁

### 代码2
```c
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id=cpuid();
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r)
    kmem[id].freelist = r->next;
    else
    for (int tmp=0;tmp<NCPU;tmp++)
           {
	    if (tmp==id) continue;
	    if (kmem[tmp].freelist)
	    {
	    acquire(&kmem[tmp].lock);
	    r=kmem[tmp].freelist;
	    kmem[tmp].freelist=r->next;
	    release(&kmem[tmp].lock);
	    break;
   	 }     
    }
    
  release(&kmem[id].lock);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
```
这个代码先判断kmem[tmp].freelist，再获取锁，因此可能在判断语句之后，因为其他cpu的状态更新导致获得kmem[tmp].lock时，kmem[tmp].freelist已经为0，会造成对空地址的访问和分配失效
