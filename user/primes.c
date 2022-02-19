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
    if (read(*out,&pri,sizeof(int))!=0)
    {
        printf("prime %d\n",pri);
        close(*in); 
        if (fork()==0){
            pipe(p); *in=p[1];
            while (read(*out,&tmp,sizeof(int))!=0)
            {
               if (tmp % pri!=0) write(*in,&tmp,sizeof(int));
            }
            close (*out); close(*in); *out=p[0];
            prime(in ,out);
        }
        else wait(0);
    }

}
