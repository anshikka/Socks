#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <zconf.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include "../include/protocol.h"

int currentConn = 0;
int totalConn = 0;
/* Locks for updating updateTable and azList */
pthread_mutex_t currentConn_lock;
pthread_mutex_t azLock;
pthread_mutex_t updateLock;
int totalEntries = 0;

typedef struct az {
    char letter;
    int count;
} letter_count;

typedef struct updates {
    int mapperId;
    int numUpdates;
    int check;
} updateStatus;


letter_count azTable[26]; // universal table of letter counts
updateStatus updateTable[MAX_CONCURRENT_CLIENTS]; // update table

// initialize struct with letters and counts = 0
void init_counts (letter_count counts[]){
	for (int i = 0; i < 26; i++){
		counts[i].letter = 65+i;
		counts[i].count = 0;
	}
}

// arguments for each thread spawned for each client
struct threadArg {
	int clientfd; // data will be passed in through client file descriptor
	char * clientip;
	int clientport;
  int clientMapperId;
};

// function to add request data to universal azTable
void add_to_counts(int request[], letter_count counts[]){
    for (int i = 0; i < 26; i++){
        counts[i].count += request[i+2];
    }
}

// sums the number of updates in the updateTable
int sumUpdates(updateStatus ustat[]){
    int total = 0;
    for (int i = 0; i < totalEntries+1; i++){
        total += ustat[i].numUpdates;
    }
    return total;

}

// fills a request/response with 0's
void fillWithZeroes(int response[]){
    for (int i = 0; i < 28; i++){
        response[i] = 0;
    }
}

// each connection runs this function
void * threadFunction (void * arg) {
    struct threadArg * tArg = (struct threadArg *) arg;
    int req[28];
    int res[28];
    int checked_in;
    int master;
    read(tArg->clientfd, req, sizeof(req));
    int reqType = req[0];
    int clientId = req[1];
    if (clientId == -1) { // check if it is a master thread
        master = 1;
    }
    else { // not a mster thread
        checked_in = 0;
        master = 0;
    }
    if (reqType == 1){ // checks in 
        fillWithZeroes(res);
        printf("[%d] CHECKIN\n", clientId);
        res[0] = reqType;
        res[2] = clientId;
        if(updateTable[clientId].check == 1 || clientId == -1){
            res[1] = RSP_NOK;
        } else {
            updateTable[clientId].mapperId = clientId;
            updateTable[clientId].check = 1; // mapper is now checked in
            updateTable[clientId].numUpdates = 0;
            totalEntries ++;
            res[1] = RSP_OK;
        }
        write(tArg->clientfd, (void *) res, sizeof(res));
        if (master) { // Extra credit: Closes master connection right after transaction
            printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
            close(tArg->clientfd);
            return NULL;
        }
    }
    while(master || updateTable[clientId].check == 1) { // can only continue when checked in
        int req1[28];
        if (!master){ // Extra credit: Closes master connection right after transaction
            read(tArg->clientfd, req1, sizeof(req1));
            reqType = req1[0];
            clientId = req[1];
        }

        switch(reqType) {
            case 2:
                fillWithZeroes(res);
                res[0] = reqType;
                if (updateTable[clientId].check == 0 || clientId == -1) {
                    res[1] = RSP_NOK;
                } else {
                    pthread_mutex_lock(&azLock);
                    add_to_counts(req1, azTable);
                    updateTable[clientId].numUpdates += 1;
                    pthread_mutex_unlock(&azLock);
                    res[1] = RSP_OK;
                    res[2] = clientId;
                }
                write(tArg->clientfd, (void *) res, sizeof(res));
                if (master){ // Extra credit: Closes master connection right after transaction
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    close(tArg->clientfd);
                    return NULL;
                }

                break;

            case 3:
                fillWithZeroes(res);
                printf("[%d] GET AZLIST\n", clientId);
                res[0] = reqType;
                if (updateTable[clientId].check == 0 && updateTable[clientId].mapperId != -1) { // if not checked in and not master
                    res[1] = RSP_NOK;
                } else { // success, get azTable
                    res[1] = RSP_OK;
                    pthread_mutex_lock(&azLock);
                    for (int i = 0; i < 26; i++){
                        res[i+2] = azTable[i].count;
                    }
                    pthread_mutex_unlock(&azLock);
                }
                write(tArg->clientfd, (void *)res, sizeof(res));
                if (master){ // Extra credit: Closes master connection right after transaction
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    close(tArg->clientfd);
                    return NULL;
                }
                break;
            case 4:
                fillWithZeroes(res);
                printf("[%d] GET_MAPPER_UPDATES\n", clientId);
                res[0] = reqType;
                if ((clientId == -1) || (updateTable[clientId].check == 0 )|| (updateTable[clientId].mapperId == 0)) { // either master or not checked in 
                    res[1] = RSP_NOK;
                } else {
                    res[1] = RSP_OK;
                    res[2] = updateTable[clientId].numUpdates;
                }
                write(tArg->clientfd, (void* )res, sizeof(res));
                if (master){ // Extra credit: Closes master connection right after transaction
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    close(tArg->clientfd);
                    return NULL;
                }
                break;
            case 5:
                fillWithZeroes(res);
                printf("[%d] GET_ALL_UPDATES\n", clientId);
                res[0] = reqType;
                if (updateTable[clientId].check == 0 && updateTable[clientId].mapperId != -1) { // either master or not checked in 
                    res[1] = RSP_NOK;
                } else { 
                    res[1] = RSP_OK;
                    res[2] = sumUpdates(updateTable);
                }
                write(tArg->clientfd, (void *) res, sizeof(res));
                if (master){ // Extra credit: Closes master connection right after transaction
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    close(tArg->clientfd);
                    return NULL;
                }
                break;
            case 6:
                fillWithZeroes(res);
                printf("[%d] CHECKOUT\n", clientId);
                res[0] = reqType;
                if ((updateTable[clientId].check == 0 && updateTable[clientId].mapperId != -1) || clientId == -1) { // either master or already checkout out
                    res[1] = RSP_NOK;
                    write(tArg->clientfd, (void*) res, sizeof(res));
                    if (master){ // Extra credit: Closes master connection right after transaction
                        printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                        close(tArg->clientfd);
                        return NULL;
                    }
                } else { // successful checkout 
                    res[1] = RSP_OK;
                    res[2] = clientId;
                    updateTable[clientId].check = 0;
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    write(tArg->clientfd, (void*) res, sizeof(res));
                    close(tArg->clientfd);
                }
                break;

            default:
                fillWithZeroes(res);
                res[0] = reqType;
                res[1] = RSP_NOK;
                res[2] = clientId;
                write(tArg->clientfd, (void *) res, sizeof(res));
                if (master){ // Extra credit: Closes master connection right after transaction
                    printf("close connection %s:%d\n", tArg->clientip, tArg->clientport);
                    close(tArg->clientfd);
                    return NULL;
                }
                break;

        }

    }
    free(tArg);


    return NULL;
}


int main(int argc, char *argv[]) {

    int server_port;

    if (argc == 2) { // 1 arguments
        server_port = atoi(argv[1]);
    } else {
        printf("Invalid or less number of arguments provided\n");
        printf("./server <server Port>\n");
        exit(0);
    }

    // Server (Reducer) code
    pthread_t threads[MAX_CONCURRENT_CLIENTS];

    // Create a TCP Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);


    // Bind it to a local address.
    struct sockaddr_in servAddress;
    servAddress.sin_family = AF_INET;
    servAddress.sin_port = htons(server_port);
    servAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr*) &servAddress, sizeof(servAddress));

    // Listen on passed in port
    listen(sock, MAX_CONCURRENT_CLIENTS);

    // initialize reducer azList
    init_counts(azTable);

    // initialize locks
    pthread_mutex_init(&currentConn_lock, NULL);
    pthread_mutex_init(&azLock, NULL);
    pthread_mutex_init(&updateLock, NULL);

    while (1) {
        // accept incoming connections
        struct sockaddr_in clientAddress;
        socklen_t size  = sizeof(struct sockaddr_in);
        int clientfd = accept(sock, (struct sockaddr*) &clientAddress, &size);
        printf("server is listening\n");

        struct threadArg *arg = (struct threadArg *) malloc(sizeof(struct threadArg));

        arg->clientfd = clientfd;
        arg->clientip = inet_ntoa(clientAddress.sin_addr);
        arg->clientport = clientAddress.sin_port;

        if (currentConn == MAX_CONCURRENT_CLIENTS) {
            printf("Server is too busy\n");
            close(clientfd);
            free(arg);
            continue;
        }
        else {
            printf("open connection from %s:%d\n", arg->clientip, arg->clientport);
            pthread_create(&threads[currentConn], NULL, threadFunction, (void *) arg);
            currentConn++;
        }
    }

    close(sock);


    return 0;
}
