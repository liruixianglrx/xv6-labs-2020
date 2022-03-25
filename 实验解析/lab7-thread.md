# LAB 7
## Uthread: switching between threads (moderate)
在`thread_switch.s`中保存和读取callee-saved寄存器，和`switch.s(kernel/switch.s)`一样
```c
sd ra, 0(a0)
        sd sp, 8(a0)
        sd s0, 16(a0)
        sd s1, 24(a0)
        sd s2, 32(a0)
        sd s3, 40(a0)
        sd s4, 48(a0)
        sd s5, 56(a0)
        sd s6, 64(a0)
        sd s7, 72(a0)
        sd s8, 80(a0)
        sd s9, 88(a0)
        sd s10, 96(a0)
        sd s11, 104(a0)

        ld ra, 0(a1)
        ld sp, 8(a1)
        ld s0, 16(a1)
        ld s1, 24(a1)
        ld s2, 32(a1)
        ld s3, 40(a1)
        ld s4, 48(a1)
        ld s5, 56(a1)
        ld s6, 64(a1)
        ld s7, 72(a1)
        ld s8, 80(a1)
        ld s9, 88(a1)
        ld s10, 96(a1)
        ld s11, 104(a1)
```

定义结构体
```c
struct tcontext {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};
```
在thread中添加
```c
struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct tcontext context;  

};
```

在`thread_schedule`中切换thread
```c
 /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
     thread_switch(（uint64）&t->contest,(uint64)&current_thread->contest);
     t->state=RUNNABLE;
```

在`thread_create`中设置ra和sp

```c
...
  // YOUR CODE HERE
  t->context.ra=(uint64)func;
  t->context.sp=(uint64)t->stack+STACK_SIZE;//point to the top of the stack
}
...
```

# Using threads

来看一下程序的运行过程：设定了五个散列桶，根据键除以5的余数决定插入到哪一个散列桶中，插入方法是头插法，下面是图示

不支持在 Docs 外粘贴 block

这个实验比较简单，首先是问为什么为造成数据丢失：

> 假设现在有两个线程T1和T2，两个线程都走到put函数，且假设两个线程中key%NBUCKET相等，即要插入同一个散列桶中。两个线程同时调用insert(key, value, &table[i], table[i])，insert是通过头插法实现的。如果先insert的线程还未返回另一个线程就开始insert，那么前面的数据会被覆盖

因此只需要对插入操作上锁即可

(1). 为每个散列桶定义一个锁，将五个锁放在一个数组中，并进行初始化

```c
pthread_mutex_t lock[NBUCKET] = { PTHREAD_MUTEX_INITIALIZER }; // 每个散列桶一把锁
```

(2). 在`put`函数中对`insert`上锁

```c
if(e){
    // update the existing key.
    e->value = value;
} else {
    pthread_mutex_lock(&lock[i]);
    // the new is new.
    insert(key, value, &table[i], table[i]);
    pthread_mutex_unlock(&lock[i]);
}
```
