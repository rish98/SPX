#include "spx_trader.h"

    int excFd;
    int traFd;
    char excPipe[256];
    char traPipe[256];
    int qty=-1;
    int orderId=0;
    int outFd;

    /* If this flag is nonzero, don’t handle the signal right away. */
    volatile sig_atomic_t signal_pending;

    /* This is nonzero if a signal arrived and was not handled. */
    volatile sig_atomic_t defer_signal;
    int excPid;


void sigAction(int sig, siginfo_t *info, void *context){
	excPid=(info->si_pid);
    char buff[100];
    read(excFd,buff,strlen(buff));
    write(outFd,buff,strlen(buff));
    write(outFd,"\n",2);
    sleep(1);

    char *cmd="BUY 0 GPU 20 10";
    write(traFd,cmd,strlen(cmd));
    kill(excPid,SIGUSR1);

    close(traFd);
    close(excFd);
    exit(0);
    
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    sleep(1);

    // register signal handler
    sigset_t block_mask;
    struct sigaction sa;
    // sigset_t oldmask;
    sigemptyset(&block_mask);
    sigaddset (&block_mask, SIGUSR1);
    // sigaddset (&block_mask, SIGCHLD);

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigAction;
    sa.sa_mask = block_mask;
    sigaction(SIGUSR1, &sa, NULL);
    //                                  connect to named pipes

    outFd=open("test.out",O_WRONLY);

    sprintf (excPipe, "%s%s", FIFO_EXCHANGE, argv[1]);
    sprintf (traPipe, "%s%s", FIFO_TRADER, argv[1]);

    if((excFd=open(excPipe,O_RDONLY))==-1){
    }
    if((traFd=open(traPipe,O_WRONLY))==-1){
    }
    
    while(1){
        pause();
        char* msg ="BUY 0 GPU 5 20";
        // sprintf(msg,"BUY %d %s %d %d",orderId++,prod,qty,price);
        write(traFd,msg,strlen(msg));
        kill(excPid,SIGUSR1);
        
        
    }
    close(traFd);
    close(excFd);
    unlink(excPipe);
	unlink(traPipe);
    exit(EXIT_SUCCESS);

    // wait for exchange update (MARKET message)
    // send order
    // wait for exchange confirmation (ACCEPTED message)
    

    
}
