#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


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
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
find(char *path,char *target)
{
  char buf[512], *p;
  int fd;
  
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    break;

  case T_DIR:
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0|| strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      char* pointbuf=buf;
      char name1[DIRSIZ+1], name2[DIRSIZ+1];
      strcpy(name1, fmtname(pointbuf));
      strcpy(name2, fmtname(target));
      if (st.type==T_DIR){
      	
      		find(buf,target);
      	
      }
      else if(!strcmp(name1,name2))
      {
        printf("%s\n",buf);
      }
    }
    break;
  }
  close(fd);
  wait(0);
}

int
main(int argc, char *argv[])
{

  if(argc < 3){
    fprintf(2,"need 3 arrangements\n",20);
    exit(1);
  }
    find(argv[1],argv[2]);
  exit(0);
}
