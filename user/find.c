#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
void findfile(char *dir,char *filename);
char* fmtname(char *path);
int main(int argc,char* argv[])
{
    if (argc<2) {
        printf("find error!");
        exit(0);
    }
    findfile(argv[1],argv[2]);
exit(0);
}
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
  
  return buf;
}
void findfile(char *dir ,char *filename)
{
    struct stat st;
    struct dirent de;
    int fd;
    char* p;
    char buf[512];
    fd= open(dir,0);
    fstat(fd,&st);
    switch (st.type)
    {
        case T_FILE:if ( strcmp(fmtname(dir),filename)==0)
                    { printf("%s\n",dir); }
                    break;
        case T_DIR:strcpy(buf,dir); 
                    p=buf+strlen(buf);
                    *p++='/';
                   while(read(fd,&de,sizeof (de)))
                   {
                     if (de.inum==0||strcmp(de.name,".")==0||strcmp(de.name,"..")==0) 
                     continue;
                     memmove(p,de.name,DIRSIZ);
                     //p[DIRSIZ] = 0;
                     findfile(buf,filename);

                   } 
                   break;               
    }
    close(fd);
}