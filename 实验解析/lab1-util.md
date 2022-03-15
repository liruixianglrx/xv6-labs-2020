# lab1 util
## sleep
编写一个用户级的sleep，很简单的实验，直接贴代码

```
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
  if (argc != 2) { //参数错误
    fprintf(2, "usage: sleep <time>\n");
    exit(1);
  }
  sleep(atoi(argv[1]));
  exit(0);
}
```
注意的是argv[0]默认是执行的文件名，在这里是sleep，argv[1]才是sleep的参数
    
## pingpong
没啥好说的，直接贴代码
```
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[])
{
    int p1[2],p2[2];
    char buf,msg='x';
    pipe(p1);
    pipe(p2);
    if (fork()!=0)
    {
        write(p1[1],&msg,1);
        read(p2[0],&buf,1);
        printf("%d: received pong\n",getpid());
    }
    else {
        read(p1[0],&buf,1);
        printf("%d: received ping\n",getpid());
        write(p2[1],&msg,1);
    }
    exit (0);
}
```

## prime
使用pipe进行通讯，pipe(p),p[0]为读取端，p[1]为写入端，如果在写入端没有全部关闭时，read(p[0])会等待数据读入，可能造成读取阻塞
因此对于不需要的管道描述符，要尽可能早的关闭。而fork()时会将读取，写入端都复制一份。

注意这里要求使用pipe不停的传入下一串素数，并用第一个数排除一些合数，代码如下
```
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void prime(int* in ,int* out );
int main()
{
    int status,i;
    int p[2],in,out;
    pipe(p);
    in=p[1]; out=p[0];
    if (fork()!=0)
    {
    for (i=2;i<=35;i++)
    write(in,&i,sizeof(int));
    close(p[1]); close(p[0]);
    wait(&status);
    }
    else {
        prime(&in,&out);
    }
exit(0);
}
void prime(int* in ,int* out)
{
    int tmp,pri,p[2];
    if (read(*out,&pri,sizeof(int))!=0)   ///读取输入流第一个数（一定是素数），判断是否还有输入流
    {
        printf("prime %d\n",pri);
        close(*in);    ///关闭fork产生的多出的读入端
        if (fork()==0){
            pipe(p); *in=p[1];
            while (read(*out,&tmp,sizeof(int))!=0)
            {
               if (tmp % pri!=0) write(*in,&tmp,sizeof(int));   //将剩下的素数写入数据流
            }
            close (*out); close(*in); *out=p[0];
            prime(in ,out);   ///递归
        }
        else wait(0);
    }

}
```

## find

