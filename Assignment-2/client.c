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
#include <pthread.h>
#include <stdlib.h>
#include <sys/wait.h>


#define BUFFER_SIZE 512
#define PORT 6069
#define NAME_SIZE 100

typedef struct _clients{
    char name[NAME_SIZE];
    pid_t cProc;
    int cSock;
    bool activeStatus;
    struct _clients* next;
}Client;

typedef struct _infoclient{
    int activeClients;
    int totalClients;
    Client* head;
}ClientQueue;

static char bmsgMsg[BUFFER_SIZE] = "BMSG";
static char umsgMsg[BUFFER_SIZE] = "UMSG";
static char joinMsg[BUFFER_SIZE] = "JOIN";
static char leaveMsg[BUFFER_SIZE] = "LEAV\r\n";
static char listMsg[BUFFER_SIZE] = "LIST\r\n";
static char* bmsgs[] = {
    "Hi, How are you loser",
};
static char* umsgs[] = {
    "yomamma",
    "Gaand Maara Sale",
    "Nidheesh Happy birthday",
    "Your mom is a cow"
};
static clock_t startTime;

void signal_handler(int signal){
    if(signal == SIGCHLD){
        // Do something
        //printf("Child Process %d is exiting\n", getpid());
        exit(0);
    }
}
//Fucked it up, rewrite
void removeProc(int cProc, ClientQueue* queue){
    Client* head = queue->head;
    while(head != NULL){
        if(head->cProc == cProc){
            head->activeStatus = false;
            queue->activeClients--;
            return;
        }
        head = head->next;
    }
}

void addCProc(Client* child, ClientQueue* queue){
    Client* head = queue->head;
    if(head == NULL){
        queue->head = child;
        queue->activeClients++;
        queue->totalClients++;
        return;
    }
    if(head->next == NULL){
        head->next = child;
        queue->activeClients++;
        queue->totalClients++;
        return;
    }
    while(head->next != NULL){
        head = head->next;
    }
    head->next = child;
    queue->totalClients++;
    queue->activeClients++;
}

void childClientProcess(int N, int M, int T, char* childName){

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1") ;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        printf("Socket didn't initialise\n");
        exit(1);
    }
    int connection = connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(connection < 0){
        printf("Failed to establish Connection\n");
        exit(1);
    }
    int sr;
    if((sr = fork()) != 0){
        // Receive Process
        signal(SIGCHLD, signal_handler);
        char recv_buff[BUFFER_SIZE];
        int bytesRcvd;
        while(1){
            bytesRcvd = read(socketfd, recv_buff, BUFFER_SIZE);
            if(bytesRcvd < 0){
                perror("Read error in client");
                break;
            }
            char msgType[5];
            msgType[0] = recv_buff[0];
            msgType[1] = recv_buff[1];
            msgType[2] = recv_buff[2];
            msgType[3] = recv_buff[3];
            msgType[4] = '\0';
            if(strcmp(msgType, "UMSG") == 0){
                printf("Received UMSG\n");
            }
            else if(strcmp(msgType, "BMSG") == 0){
                
                char* temp_buf = recv_buff+4;
                printf("Received BMSG %s\n", temp_buf);
            }
            else if(strcmp(msgType, "LIST") == 0){
                char* temp_buf = recv_buff+4;
                printf("Received LIST %s\n", temp_buf);
            }
        }
    }
    else{
        // Send Process
        char send_buff[BUFFER_SIZE];
        int bytesSent;
        int Mtemp = (M-1)/2;
        
        //First send Join Message
        strcpy(send_buff, joinMsg);
        strcat(send_buff, childName);
        strcat(send_buff, "\r\n");
        bytesSent = write(socketfd, send_buff, strlen(send_buff));
        if(bytesSent < 0){
            printf("Error while Sending");
            exit(1);
        }

        // Send LIST message
        // strcpy(send_buff, listMsg);
        // bytesSent = write(socketfd, send_buff, strlen(send_buff));
        // if(bytesSent < 0){
        //     printf("Error while Sending");
        //     exit(1);
        // }
        for(int i=1; i<4;i++){
            strcpy(send_buff, bmsgMsg);
            strcat(send_buff, bmsgs[0]);
            strcat(send_buff, "\r\n");
            bytesSent = write(socketfd, send_buff, strlen(send_buff));
            if(bytesSent < 0){
                printf("Error while Sending");
                exit(1);
            }
            printf("Send BMSG %d\n", i);
        }
    }
}

int main(int argc, char* argv[]){

    srand(5);
    if(argc <= 3){
        printf("Too few arguments, read the question\n");
        exit(1);
    }
    startTime = clock();
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    int T = atoi(argv[3]);
    char basename[NAME_SIZE] = "USER";

    int status;
    bool client_running = true;

    //Initialise ClientQueue
    ClientQueue* queue = (ClientQueue*)malloc(sizeof(ClientQueue));
    queue->activeClients = 0;
    queue->totalClients = 0;
    queue->head = NULL;

    pid_t pProc = getpid();    
    pid_t cProc;

    char childName[NAME_SIZE];
    while(client_running){ //Check end cases
        if(queue->totalClients < T){
            if(queue->activeClients < N){
                cProc = fork();
                if(cProc < 0){
                    printf("Child creation error\n");
                    exit(1);
                }
                else if(cProc == 0){
                    // Child Process
                    //sleep(1);
                    char tempNum[NAME_SIZE];
                    strcpy(childName, basename);
                    sprintf(tempNum, "%d", queue->totalClients);
                    strcat(childName, tempNum);
                    break;
                }
                else{
                    // Parent Process
                    char tempName[NAME_SIZE];
                    char tempNum[NAME_SIZE];
                    strcpy(tempName, basename);
                    sprintf(tempNum, "%d", queue->totalClients);
                    strcat(tempName, tempNum); // Check

                    Client* cli = (Client*)malloc(sizeof(Client));
                    cli->activeStatus = true;
                    cli->cProc = cProc;
                    strcpy(cli->name,tempName);
                    cli->next = NULL;
                    addCProc(cli, queue);
                    printf("Child %d is created\n", cProc);
                }
            }
            else{
                int retPid = wait(&status);
                sleep(1);
                printf("Child Process %d died\n", retPid);
                removeProc(retPid, queue);
            }
        }
        else{
            int retPid = wait(&status);
            sleep(1);
            printf("Child Process %d died\n", retPid);
            removeProc(retPid, queue);
            if(queue->activeClients == 0){
                // Do something here
                sleep(1);
                printf("Goodbye, I'm shutting down\n");
                exit(1);
            }
        }
    }
    // Child Process 
    childClientProcess(N,M,T, childName);
    return 0;
}