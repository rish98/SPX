/**
 * comp2017 - assignment 3
 * <Rishad Katrak>
 * <rkat6291>
 */
 
#include "spx_exchange.h"

//todo close file in end
static volatile int signalPid = -1;
dynTraders* traders;
dynProducts* products;
int globalId=0;
int numTraders;
double excFee=0;

int orderComp(const void *e1,const void *e2){
    order* ent1=(order *)e1;
    order* ent2=(order *)e2;
	int res=((*ent2).price)-((*ent1).price);
	if( res!=0 ){
		return res;
	}
	else{
    	return ((*ent1).globalId)-((*ent2).globalId);
	}
}
void teardown(){
	for(int i=0;i<products->size;i++){
		for(int j=0;j<products->prodArr[i]->orders->size;j++){
			// free(products->prodArr[i]->orders->orderArr[j]);
		}

		free(products->prodArr[i]->orders->orderArr);
		free(products->prodArr[i]->orders);
		free(products->prodArr[i]);
	}
	free(products->prodArr);
	free(products);
	for(int i=0;i<traders->size;i++){
		// for(int j=0;j<traders->tradArr[i].prodOrders->size;j++){
			// free(&traders->tradArr[i].prodOrders->prodOrdArr[j]);
		// }
		free(traders->tradArr[i].prodOrders->prodOrdArr);
		free(traders->tradArr[i].prodOrders);
		// free(&traders->tradArr[i]);
	}
	free(traders->tradArr);
	free(traders);


}
//https://stackoverflow.com/questions/18300077/how-poll-deal-with-closed-pipe
bool is_pipe_closed(int fd) {
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
    };

    if (poll(&pfd, 1, 1) < 0) {
        return false;
    }

    return pfd.revents & POLLERR;
}

int checkComm(order comm,int t){
	bool flag=false;
	int maxId=-1;
	for(int i=0;i<products->size;i++){
		for(int j=0;j<products->prodArr[i]->orders->size;j++){
			if(products->prodArr[i]->orders->orderArr[j].traderId==t && maxId<products->prodArr[i]->orders->orderArr[j].orderId){
				maxId=products->prodArr[i]->orders->orderArr[j].orderId;
			}
		}
	}
	if(comm.orderId!=traders->tradArr[t].maxid+1){
		flag=true;
	}
	if(comm.price<=0 || comm.qty<=0 || comm.price>999999 || comm.qty>999999){
		flag=true;
	}

	bool check=false;
	for(int i=0;i<products->size;i++){
		if(strcmp(products->prodArr[i]->name,comm.pname) == 0){
			check = true;
		}
	}

	if(check==false){
		flag=true;
	}


	if(flag==true|| check==false){
		char excPipe[256];
		char traPipe[256];
		sprintf (excPipe, "%s%d", FIFO_EXCHANGE, t);
		sprintf (traPipe, "%s%d", FIFO_TRADER, t);
		int excFd;
		if((excFd=open(excPipe,O_WRONLY))==-1){
			return 0;
		}
		write(excFd,"INVALID;", 8);
		kill(SIGUSR1,traders->tradArr[t].pid);
		return 0;
	}
	return 1;
}
void sendCommand(order comm,int t){
	char msg[50];
	sprintf (msg,"MARKET %s %s %d %d;", comm.type,comm.pname,comm.qty,comm.price);
	for(int i =0;i<numTraders;i++){	
		if(t==i){continue;}
		if(!traders->tradArr[i].cntd){return;}

		char excPipe[256];
		char traPipe[256];
		sprintf (excPipe, "%s%d", FIFO_EXCHANGE, i);
		sprintf (traPipe, "%s%d", FIFO_TRADER, i);
		int excFd;
		if((excFd=open(excPipe,O_WRONLY))==-1){
			return;

		}
		write(excFd,msg,strlen(msg));
		kill(traders->tradArr[i].pid,SIGUSR1);
	}
}
void sendFill(int tId,int orderId,int qty){
	char excPipe[256];
	int excFd;
	sprintf (excPipe, "%s%d", FIFO_EXCHANGE, tId);
	if((excFd=open(excPipe,O_WRONLY))==-1){
	}
	char buff[50];
	sprintf (buff, "FILL %d %d;", orderId,qty);
	write(excFd,buff,strlen(buff));
	kill(traders->tradArr[tId].pid, SIGUSR1);	
	close(excFd);	
}

void sendInvalid(int tId){
	char excPipe[256];
	char traPipe[256];
	sprintf (excPipe, "%s%d", FIFO_EXCHANGE, tId);
	sprintf (traPipe, "%s%d", FIFO_TRADER, tId);
	int excFd;
	if((excFd=open(excPipe,O_WRONLY))==-1){
		return;
	}
	// printf("%s",comm);
	write(excFd,"INVALID;", 8);
	kill(traders->tradArr[tId].pid,SIGUSR1);
}
void sendAccept(int tId,int orderId){
	char excPipe[256];
	int excFd;
	sprintf (excPipe, "%s%d", FIFO_EXCHANGE, tId);
	if((excFd=open(excPipe,O_WRONLY))==-1){
		return;
	}
	if(!traders->tradArr[tId].cntd){return;}
	char buff[20];
	sprintf (buff, "ACCEPTED %d;", orderId);
	write(excFd,buff,strlen(buff));
	kill(traders->tradArr[tId].pid, SIGUSR1);	
	close(excFd);	
}

void positionPrint(int tId,int orderId){
	printf("%s\t%s",LOG_PREFIX,"--POSITIONS--\n");
	for(int i=0;i<traders->size;i++){
		printf("%s\tTrader %d: ",LOG_PREFIX,i);
		for(int j=0;j<traders->tradArr[i].prodOrders->size;j++){
			if(j==traders->tradArr[i].prodOrders->size-1){
				printf("%s %d ($%0.lf)\n",traders->tradArr[i].prodOrders->prodOrdArr[j].opname,traders->tradArr[i].prodOrders->prodOrdArr[j].oqty,traders->tradArr[i].prodOrders->prodOrdArr[j].oprice);
			}
			else{
				printf("%s %d ($%0.lf), ",traders->tradArr[i].prodOrders->prodOrdArr[j].opname,traders->tradArr[i].prodOrders->prodOrdArr[j].oqty,traders->tradArr[i].prodOrders->prodOrdArr[j].oprice);
			}
		}
	}
	


	
}

void orderBookPrint(int tId,int orderId){
	printf("%s\t%s",LOG_PREFIX,"--ORDERBOOK--\n");
	for(int i=0;i<products->size;i++){
		int buyL=0;
		int sellL=0;
		int skip=0;
		for(int j=0;j<products->prodArr[i]->orders->size;j++){

			if(strcmp(products->prodArr[i]->orders->orderArr[j].type,"BUY")==0){
				buyL++;
				skip=0;
				for(int k=j+1;k<products->prodArr[i]->orders->size;k++){
					if(products->prodArr[i]->orders->orderArr[k].price==products->prodArr[i]->orders->orderArr[j].price && strcmp(products->prodArr[i]->orders->orderArr[k].type,"BUY")==0){
						skip++;
					}
				}
				j+=skip;
			}
			if(strcmp(products->prodArr[i]->orders->orderArr[j].type,"SELL")==0){
				sellL++;
				skip=0;
				for(int k=j+1;k<products->prodArr[i]->orders->size;k++){
					if(products->prodArr[i]->orders->orderArr[k].price==products->prodArr[i]->orders->orderArr[j].price && strcmp(products->prodArr[i]->orders->orderArr[k].type,"SELL")==0){
						skip++;

					}
				}
				j+=skip;
			}
		}
		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n",LOG_PREFIX,products->prodArr[i]->name,buyL,sellL);

		for(int j=0;j<products->prodArr[i]->orders->size;j++){
			int numO=1;
			int qtyO=products->prodArr[i]->orders->orderArr[j].qty;
			for(int k=j+1;k<products->prodArr[i]->orders->size;k++){
				if(products->prodArr[i]->orders->orderArr[k].price==products->prodArr[i]->orders->orderArr[j].price && strcmp(products->prodArr[i]->orders->orderArr[k].type,products->prodArr[i]->orders->orderArr[j].type)==0){
					numO++;
					qtyO+=products->prodArr[i]->orders->orderArr[k].qty;
				}
			}
			if(numO==1){
				printf("%s\t\t%s %d @ $%d (%d order)\n",LOG_PREFIX,products->prodArr[i]->orders->orderArr[j].type,qtyO,products->prodArr[i]->orders->orderArr[j].price,numO);
			}else{
				printf("%s\t\t%s %d @ $%d (%d orders)\n",LOG_PREFIX,products->prodArr[i]->orders->orderArr[j].type,qtyO,products->prodArr[i]->orders->orderArr[j].price,numO);
			}
			j+=numO-1;//todo skip next entries

		}		
	}

	positionPrint(tId,orderId);
}

void matchOrder(order comm){
	order* commPtr;
	for(int i=0;i<products->size;i++){
		for(int j=0;j<products->prodArr[i]->orders->size;j++){
			if(products->prodArr[i]->orders->orderArr[j].globalId==comm.globalId){
				commPtr=&products->prodArr[i]->orders->orderArr[j];
			}
		}
	}
	//method 2
	if(strcmp(comm.type,"BUY")==0){
		order* s;//low sell
		bool seen =false;
		for(int i=0;i<products->size;i++){
			if(strcmp(products->prodArr[i]->name,comm.pname)==0){
				for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
					if(strcmp(products->prodArr[i]->orders->orderArr[j].type,"SELL")==0){
						s=&products->prodArr[i]->orders->orderArr[j];
						seen=true;
						break;
					}
				}
			}		
		}
		if(!seen){return;}
		// printf("comm.ty;%d\n",comm.qty);

		if(comm.price>=s->price){//price matched

			double matchedPrice=s->price;//older order

			if(comm.qty>s->qty){
				double a=round((double)s->qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,s->orderId,s->traderId,comm.orderId,comm.traderId,(double)s->qty*matchedPrice,a);
				for(int i=0;i<traders->tradArr[comm.traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){
						
						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oprice+=s->qty*matchedPrice;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice-=(s->qty*matchedPrice+ a);
						
						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oqty -=s->qty;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty +=s->qty;
						sendFill(s->traderId,s->orderId,s->qty);
						sendFill(comm.traderId,comm.orderId,s->qty);
					}
				}
				commPtr->qty -= s->qty;
				comm.qty -= s->qty;
				s->qty=0;
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,s->pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(s->globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
				matchOrder(comm);
			}

			else if(comm.qty<s->qty){
				double a=round((double)comm.qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,s->orderId,s->traderId,comm.orderId,comm.traderId,(double)comm.qty*matchedPrice,a);
				
				for(int i=0;i<traders->tradArr[comm.traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){
						
						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oprice+=comm.qty*matchedPrice ;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice-=(comm.qty*matchedPrice+ a);

						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty +=comm.qty;
						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oqty -=comm.qty;
						sendFill(s->traderId,s->orderId,comm.qty);
						sendFill(comm.traderId,comm.orderId,comm.qty);
					}
				}

				s->qty -= comm.qty;
				comm.qty=0;
				commPtr->qty=0;
			}
			else if(comm.qty==s->qty){
				double a=round((double)comm.qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,s->orderId,s->traderId,comm.orderId,comm.traderId,(double)comm.qty*matchedPrice,a);
				
				for(int i=0;i<traders->tradArr[comm.traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){

						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oprice+=comm.qty*matchedPrice ;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice-=(comm.qty*matchedPrice + a);
						
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty +=comm.qty;
						traders->tradArr[s->traderId].prodOrders->prodOrdArr[i].oqty -=comm.qty;
						sendFill(s->traderId,s->orderId,s->qty);
						sendFill(comm.traderId,comm.orderId,s->qty);
					}
				}
				commPtr->qty=0;
				comm.qty=0;
				s->qty=0;	
			}
			
			//delete from orderbook if qty 0
			if(comm.qty==0){
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,comm.pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(comm.globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
			}
			if(s->qty==0){
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,s->pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(s->globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
			}
		
		}
	}

	if(strcmp(comm.type,"SELL")==0){
		order* b;//max buy 
		bool seen=false;
		for(int i=0;i<products->size;i++){
			if(strcmp(products->prodArr[i]->name,comm.pname)==0){
				for(int j=0;j<products->prodArr[i]->orders->size;j++){
					if(strcmp(products->prodArr[i]->orders->orderArr[j].type,"BUY")==0){
						b=&products->prodArr[i]->orders->orderArr[j];
						seen=true;
						break;
					}
				}
			}		
		}
		if(!seen){return;}
		if(b->price>=comm.price){//price matched
			int matchedPrice=b->price;//older order			

			if(b->qty>comm.qty){
				double a=round((double)comm.qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,b->orderId,b->traderId,comm.orderId,comm.traderId,(double)comm.qty*matchedPrice,a);
				for(int i=0;i<traders->tradArr[b->traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice+=(comm.qty*matchedPrice - a);
						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oprice-=comm.qty*matchedPrice;

						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oqty +=comm.qty;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty -=comm.qty;
						sendFill(b->traderId,b->orderId,comm.qty);
						sendFill(comm.traderId,comm.orderId,comm.qty);
					}
				}

				b->qty -= comm.qty;
				comm.qty=0;
				commPtr->qty=0;

			}
			else if(b->qty<comm.qty){
				double a=round((double)b->qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,b->orderId,b->traderId,comm.orderId,comm.traderId,(double)b->qty*matchedPrice,a);
				for(int i=0;i<traders->tradArr[b->traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){
		
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice+=(b->qty*matchedPrice - a);
						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oprice-=b->qty*matchedPrice;

						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oqty +=b->qty;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty -=b->qty;
						sendFill(b->traderId,b->orderId,b->qty);
						sendFill(comm.traderId,comm.orderId,b->qty);
					}
				}

				comm.qty -= b->qty;
				commPtr->qty -= b->qty;
				b->qty=0;
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,b->pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(b->globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
				matchOrder(comm);
			}
			else if(b->qty==comm.qty){
				double a=round((double)b->qty*matchedPrice/100);
				excFee+=a;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%0.lf, fee: $%0.lf.\n",LOG_PREFIX,b->orderId,b->traderId,comm.orderId,comm.traderId,(double)b->qty*matchedPrice,a);
				for(int i=0;i<traders->tradArr[b->traderId].prodOrders->size;i++){
					if(strcmp(traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].opname,comm.pname)==0){

						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oprice+=(comm.qty*matchedPrice - a);
						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oprice-=comm.qty*matchedPrice;

						traders->tradArr[b->traderId].prodOrders->prodOrdArr[i].oqty +=comm.qty;
						traders->tradArr[comm.traderId].prodOrders->prodOrdArr[i].oqty -=comm.qty;
						sendFill(b->traderId,b->orderId,b->qty);
						sendFill(comm.traderId,comm.orderId,b->qty);
					}
				}
				b->qty=0;
				comm.qty=0;	
				commPtr->qty=0;	
			}
			
			//delete from orderbook if qty 0
			if(b->qty==0){
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,b->pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(b->globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
			}
			if(comm.qty==0){
				for(int i=0;i<products->size;i++){
					if(strcmp(products->prodArr[i]->name,comm.pname)==0){
						for(int j=products->prodArr[i]->orders->size-1;j>=0;j--){
							if(comm.globalId==products->prodArr[i]->orders->orderArr[j].globalId){
								dynOrders_del(products->prodArr[i]->orders,j);
								break;
							}
						}
					}
				}
			}

		}

	}
}

int parseCommand(char *comm,int tId){
	//method 1
	char type[20];
	strcpy(type,strsep(&comm," "));
	if(((strcmp(type, "BUY")) == 0) || ((strcmp(type, "SELL")) == 0)){
		//ADD ORDER TO PRODUCT

		order orderObj;
		int i;
		char* temp;


		strcpy(orderObj.type,type);
		if((temp = strtok(comm, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			orderObj.orderId = atoi(temp);
		}
		if((temp = strtok(NULL, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			strcpy(orderObj.pname,temp);
		}
		if((temp = strtok(NULL, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			orderObj.qty=atoi(temp);
		}
		if((temp = strtok(NULL, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			orderObj.price=atoi(temp);
		}
		if((temp = strtok(NULL, " ")) != NULL){
			sendInvalid(tId);
			return  1;
		}
		
		orderObj.traderId=tId;
		orderObj.globalId=globalId++;
		for(i=0;i<products->size;i++){
			if(strcmp(orderObj.pname,products->prodArr[i]->name)==0){
				break;
			}
		}
		if(checkComm(orderObj,tId)==0){//check if valid comm
			//send invalid to tid
			return  1;
		}		
		traders->tradArr[tId].maxid+=1;
		dynOrders_add( products->prodArr[i]->orders,orderObj);
		sendCommand(orderObj,tId);
		//SORT ORDERS BY PRICE then global id
		qsort(products->prodArr[i]->orders->orderArr,products->prodArr[i]->orders->size,sizeof(order),orderComp);
		sendAccept(tId,orderObj.orderId);

		matchOrder(orderObj);
		orderBookPrint(tId,orderObj.orderId);
	}
	else if(strcmp(type,"CANCEL")==0){
		int cancOrd;
		char* temp;
		if((temp = strtok(comm, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			cancOrd = atoi(temp);
		}

		bool seen=false;
		for(int i=0;i<products->size;i++){
			for(int j=0;j<products->prodArr[i]->orders->size;j++){
				if(tId==products->prodArr[i]->orders->orderArr[j].traderId &&  cancOrd==products->prodArr[i]->orders->orderArr[j].orderId){
					seen=true;
					struct order orderObj;
					orderObj=products->prodArr[i]->orders->orderArr[j];
					orderObj.qty=0;
					orderObj.price=0;
					sendCommand(orderObj,tId);
					dynOrders_del(products->prodArr[i]->orders,j);
					qsort(products->prodArr[i]->orders->orderArr,products->prodArr[i]->orders->size,sizeof(order),orderComp);
					orderBookPrint(tId,-1);
					break;
				}
			}
		}
		if(!seen){
			sendInvalid(tId);
		}
		else{
			char excPipe[256];
			sprintf (excPipe, "%s%d", FIFO_EXCHANGE, tId);
			int excFd;
			if((excFd=open(excPipe,O_WRONLY))==-1){
				return  1;
			}
			char msg[50];
			sprintf (msg, "CANCELLED %d;",cancOrd);
			write(excFd,msg,strlen(msg));
			kill(SIGUSR1,traders->tradArr[tId].pid);
		}


	}
	else if(strcmp(type,"AMEND")==0){
		//CHECK VALID AMMEND
		char* temp;
		int ammOrd=-1;
		int ammQty=-1;
		int ammPrice=-1;
		if((temp = strtok(comm, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			ammOrd = atoi(temp);
		}
		if((temp = strtok(NULL, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			ammQty = atoi(temp);
		}
		if((temp = strtok(NULL, " ")) == NULL){
			sendInvalid(tId);
			return  1;
		}else{
			ammPrice = atoi(temp);
		}


		int seen=0;

		if((ammQty<=0) || (ammQty>999999) || (ammPrice<=0) || (ammPrice>999999)){
			seen=-1;
		}
		if(seen == 0){
		for(int i=0;i<products->size;i++){
			for(int j=0;j<products->prodArr[i]->orders->size;j++){
				if((tId==products->prodArr[i]->orders->orderArr[j].traderId) && (ammOrd==products->prodArr[i]->orders->orderArr[j].orderId)){
					seen=1;
					order orderObj;
					products->prodArr[i]->orders->orderArr[j].qty=ammQty;
					products->prodArr[i]->orders->orderArr[j].price=ammPrice;
					products->prodArr[i]->orders->orderArr[j].globalId=globalId++;
					orderObj=products->prodArr[i]->orders->orderArr[j];
					qsort(products->prodArr[i]->orders->orderArr,products->prodArr[i]->orders->size,sizeof(order),orderComp);
					matchOrder(orderObj);
					orderBookPrint(tId,-1);
					sendCommand(orderObj,tId);
					break;

				}
			}
			
		}}

		if(seen <= 0){
			sendInvalid(tId);
		}
		else{
			char excPipe[256];
			sprintf (excPipe, "%s%d", FIFO_EXCHANGE, tId);
			int excFd;
			if((excFd=open(excPipe,O_WRONLY))==-1){
				return  1;

			}
			char msg[50];
			sprintf (msg, "AMENDED %d;",ammOrd);
			write(excFd,msg,strlen(msg));
			kill(SIGUSR1,traders->tradArr[tId].pid);

		}
	}
	return 0;
}

void closeTrader(int signalPid){
	int t=0;
	while(t<traders->size){
		if(traders->tradArr[t].pid==signalPid){
			if(traders->tradArr[t].cntd==false){
				return;
			}
			traders->tradArr[t].cntd=false;
			break;
		}
		t++;
	}
	printf("%s Trader %d disconnected\n",LOG_PREFIX,t);
	char excPipe[256];
	char traPipe[256];
	sprintf (excPipe, "%s%d", FIFO_EXCHANGE, t);
	sprintf (traPipe, "%s%d", FIFO_TRADER, t);

	unlink(excPipe);
	unlink(traPipe);
	numTraders--;

}

void getCommand(int signalPid){
	//READ FROM TRADER
    int t=0;
	while(t<traders->size){
		if(traders->tradArr[t].pid==signalPid){
			break;
		}
		t++;
	}
	char traPipe[256];
	int traFd;
	char comm[44];
	char buff[1];
	int ret;
	sprintf (traPipe, "%s%d", FIFO_TRADER, t);
	if((traFd=open(traPipe,O_RDONLY))==-1){
		// printf("%s","cant open read pipe1");
		// closeTrader(traders->tradArr[t].pid);
		return;
	}
	int i=0;
	while(i<43){
		ret=read(traFd,buff,1);
		if(ret==0){
			printf("%s","got 0 ret in read");
			break;
		}
		if(buff[0]==';'){
			break;
		}
		comm[i]=buff[0];
		i++;
	}
	comm[i]='\0';
	printf("%s [T%d] Parsing command: <%s>\n",LOG_PREFIX,t,comm);//todo check if valid command
	parseCommand(comm,t);
	
}

void sigActionChld(int sig, siginfo_t *info, void *context){
	closeTrader(info->si_pid);
}

void sigAction(int sig, siginfo_t *info, void *context){
	getCommand(info->si_pid);
}

int sigHandler(int sig){
	printf("caught sig");
	return 1;
}

#ifndef TESTING

int main(int argc, char **argv) {
	// signal(SIGUSR1,sigHandler);
	traders=dynTraders_init();

	numTraders= argc-2;
	FILE *fptr;
	products=dynProducts_init();
	char prodTemp[20];

	if((fptr=fopen(argv[1],"r"))==NULL){
		write(1,"cant open product file\n",24);
		return -1;
	}

	printf("%s Starting\n",LOG_PREFIX);

	if(fgets(prodTemp,20,fptr)==NULL){
		printf("%s","No number in file");
	}
	else{
		strtok(prodTemp,"\n");
		printf("%s Trading %s products:",LOG_PREFIX,prodTemp);
	}
	//ADD PRODUCT TO LIST
	while(fgets(prodTemp,sizeof(prodTemp),fptr)!=NULL){
		printf(" %s",strtok(prodTemp,"\n"));		
		product* prodObj=(product*)malloc(sizeof(product));
		strcpy(prodObj->name,prodTemp);
		prodObj->orders=dynOrders_init();
		dynProducts_add(products,prodObj);//malloc
	}
	printf("\n");

	//START UP
	for(int i =0;i<numTraders;i++){
		char excPipe[256];
		char traPipe[256];
		sprintf (excPipe, "%s%d", FIFO_EXCHANGE, i);
		sprintf (traPipe, "%s%d", FIFO_TRADER, i);
		
		if (mkfifo(excPipe,0777)==-1){
			return -1;

			// write(1,"error mkfifo excPipe; ",23);
		}
		printf("%s Created FIFO %s\n",LOG_PREFIX,excPipe);

		if(mkfifo(traPipe,0777)==-1){
			return -1;

			// write(1,"error mkfifo traPipe; ",23);
		}
		printf("%s Created FIFO %s\n",LOG_PREFIX,traPipe);

		int pid=fork();
		if(pid==0){//child
			char str[20];
			sprintf(str, "%d", i);
			printf("%s Starting trader %s (%s)\n",LOG_PREFIX,str,argv[i+2]);
			execlp(argv[i+2],argv[i+2],str,(char*)NULL);
		}
		else{//parent
			dynTraders_add(traders,pid);//making and adding trader struct with that pid
			for(int i=0;i<products->size;i++){
				struct prodOrder poTemp;
				poTemp.oprice=0;
				poTemp.oqty=0;
				strcpy(poTemp.opname,products->prodArr[i]->name);
				dynProdOrders_add(traders->tradArr[traders->size-1].prodOrders,poTemp);
			}
			
			int excFd;
			int traFd;
			if((excFd=open(excPipe,O_WRONLY))==-1){
				return -1;
			}
			printf("%s Connected to %s\n",LOG_PREFIX,excPipe);
			write(excFd,"MARKET OPEN;",12);
			kill(pid, SIGUSR1);	
			// close(excFd);

			if((traFd=open(traPipe,O_RDONLY))==-1){
				printf("%s","error raeding 1\n");
				return -1;
			}
			printf("%s Connected to %s\n",LOG_PREFIX,traPipe);
		}
		
			
	}
	

	
		sigset_t block_mask;
		struct sigaction sa;
		struct sigaction sc;
		// sigset_t oldmask;
		sigemptyset(&block_mask);
		sigaddset (&block_mask, SIGUSR1);
		sigaddset (&block_mask, SIGCHLD);

		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = sigAction;
		sa.sa_mask = block_mask;
		sigaction(SIGUSR1, &sa, NULL);

		sc.sa_flags = SA_SIGINFO;
		sc.sa_sigaction = sigActionChld;
		sc.sa_mask = block_mask;
		sigaction(SIGCHLD, &sc, NULL);
	while(1){
		

		// sleep(1);                           //todo wait for a signal 
		pause();
		int t=0;								//polling (2)
		while(t<traders->size){
			if(is_pipe_closed(traders->tradArr[t].pid)){
				closeTrader(traders->tradArr[t].pid);
			}
			t++;
		}		

		if (numTraders==0){
			printf("%s Trading completed\n",LOG_PREFIX);
			printf("%s Exchange fees collected: $%.0lf\n",LOG_PREFIX,excFee);
			teardown();
			return 0;
		}
	}

	return 1;
}
#endif

