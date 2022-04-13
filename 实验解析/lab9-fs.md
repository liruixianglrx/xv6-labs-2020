# LAB9 
## Large files

在`fs.h`中，改变NDIRECT，ADDRS，maxfile的定义,addrs[11]作为一级间接块，addrs[12]作为二级间接块
添加NNINDIRECT作为二级间接块数目
```
...
#define NDIRECT 11
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT+NINDIRECT*NINDIRECT) //11+256+256*256
#define NNINDIRECT (NINDIRECT*NINDIRECT) //256*256
...
uint addrs[NDIRECT+1+1];
```c

改变bmap，添加第二级间接块的映射

```
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;
  struct buf *bp2;//the second level of the block 

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
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
```c
