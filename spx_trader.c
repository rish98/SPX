#include "spx_trader.h"
    int excFd;
    int traFd;
    char excPipe[256];
    char traPipe[256];
    int qty=-1;
    int orderId=0;
    //https://www.gnu.org/software/libc/manual/html_node/Remembering-a-Signal.html
    /* If this flag is nonzero, don’t handle the signal right away. */
    volatile sig_atomic_t signal_pending;

    /* This is nonzero if a signal arrived and was not handled. */
    volatile sig_atomic_t defer_signal;
    int excPid;

void sendAuto(){
    defer_signal++;
    char buff[1];
    char comm[44];
    int i=0;
    int ret;
	while(i<43){
		ret=read(excFd,buff,1);
		if(ret==0){
			// printf("%s","got 0 ret in read");
			break;
		}
		if(buff[0]==';'){
			break;
		}
		comm[i]=buff[0];
		i++;
	}
	comm[i]='\0';
	// printf("Parsing command: <%s>\n",comm);
    
    char *a;
    a=strtok(comm," ");
    // int qty=-1;
    int price=-1;
    if(strcmp(a,"MARKET")==0){
        char *temp=strtok(NULL," ");
        if(strcmp(temp,"SELL")==0){
            printf("%s",comm);
            char *prod=strtok(NULL," ");
            qty=atoi(strtok(NULL," "));
            price=atoi(strtok(NULL," "));

            if(qty<1000){
                char msg[100];
                sprintf(msg,"BUY %d %s %d %d",orderId++,prod,qty,price);
                write(traFd,msg,strlen(msg));
                kill(excPid,SIGUSR1);
                int ret =sleep(1);
                if(ret!=-1){
                    kill(excPid,SIGUSR1);//should read accept
                    sleep(2);
                }
                kill(excPid,SIGUSR1);
                // sleep(1);
                // kill(pid,SIGUSR1);
                // sleep(1);
            }
        }
    }
    defer_signal--;
    if (defer_signal == 0 && signal_pending != 0)
        raise (signal_pending);

            // char *msg="BUY 0 TSLA 50 987;";
          
    // close(traFd);
    // close(excFd);
    // unlink(excPipe);
	// unlink(traPipe);
}
void sigAction(int sig, siginfo_t *info, void *context){
	excPid=(info->si_pid);
    if (defer_signal)
    signal_pending = SIGUSR1;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }


    // register signal handler
    sigset_t block_mask;
    struct sigaction sa;
    sigemptyset(&block_mask);
    sigaddset (&block_mask, SIGUSR1);

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sigAction;
    sa.sa_mask = block_mask;
    sigaction(SIGUSR1, &sa, NULL);     //connect to named pipes

    sprintf (excPipe, "%s%s", FIFO_EXCHANGE, argv[1]);
    sprintf (traPipe, "%s%s", FIFO_TRADER, argv[1]);

    if((excFd=open(excPipe,O_RDONLY))==-1){
    }
    if((traFd=open(traPipe,O_WRONLY))==-1){
    }
    // int flags = fcntl(excFd, F_GETFL, 0);
    // fcntl(excFd, F_SETFL, flags | O_NONBLOCK);

    while(1){
        pause();
        sendAuto();
        // sleep(2);
        if(qty>=1000){
            break;
        }
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

//YESS!
// ٩( ͡° ͜ʖ ͡°)ง