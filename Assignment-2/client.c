#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h> 
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<pthread.h>

#define BUFFER_SIZE 512
#define PORT 6069
#define NAME_SIZE 100

typedef struct _clients{
    char name[NAME_SIZE];
    pid_t cProc;
    int cSock;
    bool activeStatus;
}Client;

typedef struct _infoclient{
    int aClients;
    int inClients;
    Client* head;
}ClientQueue;

static char joinMsg[] = "JOIN";
static char* bmsgs[] = {
    "Hi, How are you loser",
    "Shenoy you drunkard",
    "Mamallan you hoe, do assignment",
    "Naveen, do VAANI instead",
    "Benny, you are a monster",
    "Aswin you cow",
    "Mustansir"
};
static char* umsgs[] = {
    "yomamma",
    "Gaand Maara Sale",
    "Nidheesh Happy birthday",
    "Your mom is a cow"
};
static clock_t startTime;

int main(int argc, char* argv[]){
    if(argc <= 3){
        printf("Too few arguments, read the question\n");
        exit(1);
    }
    startTime = clock();
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    int T = atoi(argv[3]);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1") ;
    bool client_running = true;
    int ac_clients = 0;
    int to_clients = 0;

    //Initialise ClientQueue

    pid_t pProc = getpid();    
    pid_t cProc;

    while(client_running){
        if(ac_clients <= N){
            cProc = fork();
            if(cProc == 0){
                // Child Process
                break;
            }
        }
    }
    return 0;
}