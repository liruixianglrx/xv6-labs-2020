# lab6-cow
## copy-on-write

在vm.c中添加数组`int ref[32768];`用于指示引用物理内存页的页表数

32768是因为PHYSTOP是0x86400000,KERNBASE是0x80000000，两者相减除以PGSIZE得到的
其中 ref[i]表示索引物理地址为i*PGSIZE+KERNBASE 的页表数目

根据提示修改uvmcopy，不再为新页表分配内存
```
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  /*char *mem;
  
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }*/

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
     *pte=*pte & ~PTE_W;
     pa =PTE2PA(*pte);
     flags=PTE_FLAGS(*pte);
     
     if(mappages(new,i,PGSIZE,(uint64)pa,flags) !=0){
     goto err;
     }
     }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}
```


于riscv.h中
加入标识位PTE_F表示是否fork调用,使用系统预留标识位
`#define PTE_F (1L << 8)`
![image](https://user-images.githubusercontent.com/99662709/158755684-dc8394b4-866b-4462-ba6b-9d9a53e72e4d.png)

