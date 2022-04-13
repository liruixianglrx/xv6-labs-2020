# LAB9 
## Large files

在`fs.h`中，改变NDIRECT，ADDRS，maxfile的定义,addrs[11]作为一级间接块，addrs[12]作为二级间接块
添加NNINDIRECT作为二级间接块数目
```c
...
#define NDIRECT 11
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT+NINDIRECT*NINDIRECT) //11+256+256*256
#define NNINDIRECT (NINDIRECT*NINDIRECT) //256*256
...
uint addrs[NDIRECT+1+1];
```

修改内存中的inode
```c
struct inode {
...
  short nlink;
  uint size;
  uint addrs[NDIRECT+1+1];
};
```

改变bmap，添加第二级间接块的映射

```c
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;
  struct buf *bp2;//the second level of the block 

  if(bn < NDIRECT){
 ...
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
   ...
  }
  
  
  bn -=NINDIRECT;
  if (bn<NNINDIRECT) {
  if ((addr=ip->addrs[12])==0)
      ip->addrs[12]=addr=balloc(ip->dev); //allocate if necessary
    bp=bread(ip->dev,addr); //read the first level of the block
    a=(uint*)bp->data;
    
    if  ((addr =a[bn /256])==0){
       a[bn/256]=addr=balloc(ip->dev);  //allocate if necessary
       log_write(bp); //content of bp has changed,write to the log
    }
    bp2=bread(ip->dev,addr);
    a=(uint*)bp->data;
    
    if ((addr=a[bn %256])==0){
       a[bn%256]=addr=balloc(ip->dev);
       log_write(bp);
    }
    brelse(bp);
    brelse(bp2);
    return addr;
  }

  panic("bmap: out of range");
}
```
注意更好的实现应该是在修改完成bp后立刻释放bp的锁，因为以后的修改都是修改其他block，这样做能提高部分性能，本人此处为了代码整体美观没有这么做

修改itrunc以释放所有的块，和之前换汤不换药
```c
void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  struct buf *bp1;
  uint *a;

  for(i = 0; i < NDIRECT; i++){
  ...
  }

  if(ip->addrs[NDIRECT]){
  ...
  }
  
  
  uint* a1;
  if(ip->addrs[NDIRECT + 1]) {
    bp = bread(ip->dev, ip->addrs[NDIRECT + 1]);
    a = (uint*)bp->data;
    for(i = 0; i < 256; i++) {
      if(a[i]) {
        bp1 = bread(ip->dev, a[i]);
        a1 = (uint*)bp1->data;
        for(j = 0; j < 256; j++) {
          if(a1[j])
            bfree(ip->dev, a1[j]);
        }
        brelse(bp1);
        bfree(ip->dev, a[i]);
      }
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT + 1]);
    ip->addrs[NDIRECT + 1] = 0;
  }

  
  ip->size = 0;
  iupdate(ip);
}
```
