# LAB 10
## mmap
首先添加各种定义以便能调用syscall`mmap`和`munmap`，在这里就不赘诉了

定义结构体
```c
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
在struct proc里添加结构
```c
struct proc {
...
  char name[16];               // Process name (debugging)
  struct vmarea vma[NVMA];
};
...
