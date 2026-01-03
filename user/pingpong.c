#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p_parent[2],p_child[2];
  pipe(p_parent);
  pipe(p_child);
  char buf[1];
  if(fork()==0)
  {
	close(p_parent[1]);  	
  	close(p_child[0]);
	if(read(p_parent[0],buf,1))
  		fprintf(1,"%d: received ping\n",getpid());
	write(p_child[1],"B",1);
  	close(p_child[1]);
  }else
  {
	close(p_parent[0]);
	close(p_child[1]);
  	write(p_parent[1],"A",1);
  	if(read(p_child[0],buf,1))
  		fprintf(1,"%d: received pong\n",getpid());
  		
  	close(p_parent[1]);
  }
  exit(0);
}
