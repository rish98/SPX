#ifndef SPX_COMMON_H
#define SPX_COMMON_H

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>

#define FIFO_EXCHANGE "/tmp/spx_exchange_"
#define FIFO_TRADER "/tmp/spx_trader_"
#define FEE_PERCENTAGE 1

typedef struct prodOrder{
    double oprice;
    int oqty;
    char opname[17];
}prodOrder;

typedef struct dynProdOrders{
    struct prodOrder* prodOrdArr;
    int size;
    int capacity;
}dynProdOrders;

typedef struct trader{
    int pid;
    int maxid;
    bool cntd;
    struct dynProdOrders* prodOrders;
}trader;

typedef struct dynTraders{
    trader* tradArr;
    int size;
    int capacity;
}dynTraders;

typedef struct product{
    char name[17];
    struct dynOrders* orders;
}product;

typedef struct dynProducts{
    struct product** prodArr;
    int size;
    int capacity;
}dynProducts;

typedef struct dynOrders{
    struct order* orderArr;
    int size;
    int capacity;
}dynOrders;

typedef struct order{
    char pname[17];
    char type[17];
    int orderId;
    int qty;
    int price;
    int traderId;
    int globalId;
}order;

dynProducts* dynProducts_init(){
    dynProducts* my_entArr = malloc(sizeof(dynProducts));
    my_entArr -> size = 0;

    my_entArr -> prodArr = malloc(sizeof(product*) * 5);
    my_entArr -> capacity = 5;

    return my_entArr;
}

void dynProducts_add( dynProducts* dyn, product* ent){
    if (NULL == dyn){return;}

    if (dyn->size + 1 > dyn->capacity ){
        // realloc
        dyn->prodArr = realloc(dyn->prodArr, (dyn->capacity + 15) * sizeof(product*));
        dyn->capacity += 15;
    }
    dyn->size ++;
    (dyn->prodArr)[dyn->size - 1] = ent;
    return;
}

void dynProducts_free( dynProducts* dyn){
	free(dyn->prodArr);
	free(dyn);
}

dynOrders* dynOrders_init(){
    dynOrders* my_entArr =(dynOrders*) malloc(sizeof(dynOrders));
    my_entArr -> size = 0;

    my_entArr -> orderArr = malloc(sizeof(order) * 5);
    my_entArr -> capacity = 5;

    return my_entArr;
}

void dynOrders_add( dynOrders* dyn, order ent){
    if (NULL == dyn){return;}

    if (dyn->size + 1 > dyn->capacity ){
        // realloc
        dyn->orderArr = realloc(dyn->orderArr, (dyn->capacity + 15) * sizeof(order));
        dyn->capacity += 15;
    }
    dyn->size ++;
    (dyn->orderArr)[dyn->size - 1] = ent;
    return;
}

void dynOrders_del(dynOrders* dyn,int index){
    if (NULL == dyn || index >= dyn->size){return;}
    // free(*(dyn ->orderArr + index));                             //todo free order
    // free(dyn->orderArr[index]);
    for (int i = index; i < dyn->size - 1; i++){
        *(dyn ->orderArr + i) = *(dyn ->orderArr + i + 1);
    }
    dyn -> size --;
}

void dynOrders_free( dynOrders* dyn){
	free(dyn->orderArr);
	free(dyn);
}

dynTraders* dynTraders_init(){
    dynTraders* my_entArr =(dynTraders*) malloc(sizeof(dynTraders));
    my_entArr -> size = 0;

    my_entArr ->tradArr = malloc(sizeof(trader) * 5);
    my_entArr -> capacity = 5;

    return my_entArr;
}

dynProdOrders* dynProdOrders_init(){
    dynProdOrders* my_entArr =(dynProdOrders*) malloc(sizeof(dynProdOrders));
    my_entArr -> size = 0;

    my_entArr ->prodOrdArr = malloc(sizeof(prodOrder) * 5);
    my_entArr -> capacity = 5;

    return my_entArr;
}

void dynProdOrders_add( dynProdOrders* dyn, prodOrder ent){
    if (NULL == dyn){return;}

    if (dyn->size + 1 > dyn->capacity ){
        // realloc
        dyn->prodOrdArr = realloc(dyn->prodOrdArr, (dyn->capacity + 15) * sizeof(prodOrder));
        dyn->capacity += 15;
    }
    dyn->size ++;
    (dyn->prodOrdArr)[dyn->size - 1] = ent;
    return;
}

void dynProdOrders_free( dynProdOrders* dyn){
	free(dyn->prodOrdArr);
	free(dyn);
}

void dynTraders_add( dynTraders* dyn, int ent){
    if (NULL == dyn){return;}
    trader tra={.pid=ent,.cntd=true,.maxid=-1};
    tra.prodOrders=dynProdOrders_init();

    if (dyn->size + 1 > dyn->capacity ){
        // realloc
        dyn->tradArr = realloc(dyn->tradArr, (dyn->capacity + 15) * sizeof(trader));
        dyn->capacity += 15;
    }
    dyn->size ++;
    (dyn->tradArr)[dyn->size - 1] = tra;
    return;
}

void dynTraders_free( dynTraders* dyn){
	free(dyn->tradArr);
	free(dyn);
}

#endif