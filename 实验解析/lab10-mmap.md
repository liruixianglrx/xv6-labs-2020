# LAB 10
## mmap
首先添加各种定义以便能调用syscall`mmap`和`munmap`，在这里就不赘诉了

定义结构体
```c
#define NVMA 16
struct vmarea{
uint64 addr;
int len;
int prot;
int flags;
int fd;
int offset;
struct file * fl;
int used;

};
```

在struct proc里添加结构,用于存放虚拟内存的各种信息
```c
struct proc {
...
  char name[16];               // Process name (debugging)
  struct vmarea vma[NVMA];
};
```

根据提示在`sysfile.c`里编写`sys_mmap`进行懒分配,记得根据提示增加对文件的引用计数，否则关闭结构体后可能导致文件消失
```c
uint64
sys_mmap(void)
{
uint64 addr;
int len;
int prot;
int flags;
int fd;
int offset;
struct file* fl;
uint64 err=0xffffffffffffffff;

if (argaddr(0,&addr)<0||argint(1,&len)<0||argint(2,&prot)<0
    ||argint(3,&flags)<0|| argfd(4,&fd,&fl)<0||argint(5,&offset)<0)
return err;

struct proc *p=myproc();

for (int i=0;i<NVMA;i++)
{
   if (p->vma[i].used==0)
   {
     p->vma[i].used=1;
     p->vma[i].addr=p->sz;
     p->vma[i].len=len;
     p->vma[i].prot=prot;
     p->vma[i].flags=flags;
     p->vma[i].fd=fd;
     p->vma[i].offset=offset;
     p->vma[i].fl=fl;
    
    filedup(fl);
     p->sz+=len;
     return p->vma[i].addr;
   }
}

return err;     
}
```
此处对于各种错误情况的处理不是很好，查看了下其他大佬的写法处理的很不错，在这里贴一个样例
```c
uint64
sys_mmap(void) {
  uint64 addr;
  int length;
  int prot;
  int flags;
  int vfd;
  struct file* vfile;
  int offset;
  uint64 err = 0xffffffffffffffff;

  // 获取系统调用参数
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0 ||
    argint(3, &flags) < 0 || argfd(4, &vfd, &vfile) < 0 || argint(5, &offset) < 0)
    return err;

  // 实验提示中假定addr和offset为0，简化程序可能发生的情况
  if(addr != 0 || offset != 0 || length < 0)
    return err;

  // 文件不可写则不允许拥有PROT_WRITE权限时映射为MAP_SHARED
  if(vfile->writable == 0 && (prot & PROT_WRITE) != 0 && flags == MAP_SHARED)
    return err;

  struct proc* p = myproc();
  // 没有足够的虚拟地址空间
  if(p->sz + length > MAXVA)
    return err;

  // 遍历查找未使用的VMA结构体
  for(int i = 0; i < NVMA; ++i) {
    if(p->vma[i].used == 0) {
      p->vma[i].used = 1;
      p->vma[i].addr = p->sz;
      p->vma[i].len = length;
      p->vma[i].flags = flags;
      p->vma[i].prot = prot;
      p->vma[i].vfile = vfile;
      p->vma[i].vfd = vfd;
      p->vma[i].offset = offset;

      // 增加文件的引用计数
      filedup(vfile);

      p->sz += length;
      return p->vma[i].addr;
    }
  }

  return err;
}
```

做到此处应该已经能成功引起pagefault,运行mmaptest后也果然如此

在`usertrap`里处理pagefault，添加mmaphandler
```c
void
usertrap(void)
{
 ...
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if (r_scause()==13||r_scause()==15)
  {
  #ifdef LAB_MMAP
  uint64 va=r_stval();
  if (va<p->sz) 
     {if (mmap_handler(va,r_scause()) !=0) p->killed=1;}
  else p->killed=1;   
  
  #endif 
  }  
  
  else {
  ...
  ```
  
  函数`mmap_handler()`
  ```c
  int mmap_handler(uint64 va,int cause) 
{
uint64* pa;
int i;

if ((pa=kalloc())==0) return -1;
memset(pa,0,PGSIZE);

struct proc *p=myproc();
for (i=0;i<NVMA;i++)
   if (p->vma[i].addr==va) break;   ///find the file

if (i==NVMA) return -1;   
 
int flags=PTE_U;
if (p->vma[i].prot&PROT_READ ) flags=flags|PTE_R;
if (p->vma[i].prot&PROT_WRITE) flags=flags|PTE_W;
if (p->vma[i].prot&PROT_EXEC) flags=flags|PTE_X;

if (mappages(p->pagetable,PGROUNDDOWN(va),PGSIZE,(uint64)pa,flags)!=0)
{
kfree(pa);
return -1;
};

struct file* fl=p->vma[i].fl;
ilock(fl->ip);
readi(fl->ip,1,(uint64)pa,p->vma[i].offset,p->vma[i].len);
iunlock(fl->ip);

return 0;
}
  ```
