#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define prime_max_tmp 35
void get_prime(int *p)
{
    int primeout=0;
    int buf[1];
    close(p[1]);
    if(read(p[0],buf,4))
    {
        primeout=buf[0];
        fprintf(1,"prime %d\n",primeout);
    }
    else
    {
    	exit(0);
    }
        
    int  p_child[2];
    pipe(p_child);
    if(fork()==0)
    {
        close(p_child[1]);
        get_prime(p_child);
        close(p_child[0]);
        exit(0);
    }
    else{
        close(p_child[0]);
        while(read(p[0],buf,4))
        {
            if(buf[0]%primeout!=0)
                write(p_child[1],buf,4);
        }
        close(p[0]);
        close(p_child[1]);//重要
        wait(0);
    	exit(0);
    }
    exit(0);
}
int main(int argc, char *argv[])
{
     
        int p_parent[2];
        pipe(p_parent);
        if(fork()==0)
        {
            get_prime(p_parent);
            exit(0);
        }
        else
        {
            close(p_parent[0]);
            for(int i=2;i<=prime_max_tmp;i++)
                write(p_parent[1],&i,4);
            close(p_parent[1]);
            wait(0);
            exit(0);
        }
        
}
