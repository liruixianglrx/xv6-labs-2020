# sleep
编写一个用户级的sleep，很简单的实验，直接贴代码

```c
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
    
