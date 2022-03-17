# lab6-cow
## copy-on-write

在vm.c中添加数组`int ref[32768];`用于指示引用物理内存页的页表数

32768是因为PHYSTOP是0x86400000,KERNBASE是0x80000000，两者相减除以PGSIZE得到的
其中 ref[i]表示索引物理地址为i*PGSIZE+KERNBASE 的页表数目



