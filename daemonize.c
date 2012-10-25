#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static void daemonize(int min)
{
    pid_t pid, sid;
    time_t current_time;
    char buffer[255];
    memset(buffer,-1,sizeof(buffer));

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/home/log", "a", stdout);
    freopen( "/dev/null", "w", stderr);
    
    while(1){
	 time(&current_time);
         swaptime(current_time, buffer);
         fprintf(stdout, "%s Hello world!!\n", buffer);
	 fflush(stdout);
	 sleep(min);
    }
    
}
void swaptime(time_t org_time, char *time_str){

    struct tm *tm_ptr;
    tm_ptr = gmtime(&org_time);
    sprintf(time_str, "%d-%d-%d %d:%d:%d", tm_ptr->tm_year+1900,
                    tm_ptr->tm_mon+1,
                    tm_ptr->tm_mday,
                    tm_ptr->tm_hour,
                    tm_ptr->tm_min,
                    tm_ptr->tm_sec);
}

int main( int argc, char *argv[] ) {

    int time_min;
    int flag;
    int param;
    
    if(argc == 1)
        printf("need param!!\n");


    while( (param = getopt(argc, argv, ":t:")) != -1 ){
        switch(param){		
            case 't':
                time_min = atoi(optarg);
                
		printf("%d\n",time_min);
		flag = 1;
		break;
            case '?':
                printf("Unknow param\n");
		break;
            case ':':
                printf("Miss param\n");
		break; 
        }
    }

    if(flag == 1)
        daemonize(time_min);

    return 0;
}


