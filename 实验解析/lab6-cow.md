# lab6-cow
## copy-on-write

在vm.c中添加数组`int ref[32768];`用于指示引用物理内存页的页表数

32768是因为PHYSTOP是0x86400000,KERNBASE是0x80000000，两者相减除以PGSIZE得到的
其中 ref[i]表示索引物理地址为i*PGSIZE+KERNBASE 的页表数目

根据提示修改uvmcopy，不再为新页表分配内存
**注意修改新旧页表的PTE_W位**
```c
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

修改usertrap
```c
} else if((which_dev = devintr()) != 0){
    // ok
  } else if(r_scause==13 ||r_scause()==15){
     pte_t *pte;
     uint64 addr=r_stval();
     addr=PGROUNDDOWN(addr);
     if ((pte=walk(p->pagetable,addr,0))==0||!(*pte & PTE_COW))
     {p->killed=1;}
     else {
           char *mem;
           uint64 pa=PTE2PA(*pte);
           if (ref[(pa-KERBASE)/PGSIZE]==2){
              *pte=*pte | PTE_W;
              *pte=*pte & ~PTE_COW;
     	  }else if ( (mem=kalloc())==0){ p->killed;}
     	   else {
     	    	ref[(pa-KERBASE)/PGSIZE]-=1;
     	    	memmove(mem,(char*)pa,PGIZE);
     	    	*pte=*pte | PTE_W;
     	    	*pte=*pte & ~PTE_COW;
     	    	flags=PTE_FLAGS(*pte);
     	    	mappages(p->pagetable,addr,PGSIZE,mem,flags);
     	    	ref[(mem-KERBASE)/PGSIZE]+=1;
     	  	}
  }
  }
  ...
```

在`kalloc（）`中为ref设初值
```c
if(r)
   { memset((char*)r, 5, PGSIZE); // fill with junk
     ref[(r-KERBASE)/PGSIZE]=1;
   }
```
