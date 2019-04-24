// P_Threads
// Compiling gcc pthread.c -lpthread
// Discuss what else can be added to this
// (i) Leaving and Joining Messages?
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
#define MAX_CLIENTS 20
#define NAME_SIZE 100

// Initialize the data structures
typedef struct _client{
    int socket;
    char name[100];
    struct sockaddr_in si_other;
}Client;

static Client* queue[MAX_CLIENTS];
int static clientID = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void removeClient(int socketfd){
    for(int i=0; i<clientID;i++){
        Client* cli = queue[i];
        Client* freeMem;
        if(cli->socket == socketfd){
            freeMem = cli;
            if(i == clientID-1){
                free(freeMem);
            }
            for(int j=i; j<clientID-1;j++){
                queue[i] = queue[i+1];
            }
            clientID--;
        }
    }
}

void strEcho(int socketfd){
    int readSize;
    char* recv_buf;
    int endOfMsg = 1;
    int bytesRead = 0;
    while(1){
        if(endOfMsg == 1){
            bytesRead = 0;
            endOfMsg = 0;
            recv_buf = (char*)malloc(sizeof(char)*BUFFER_SIZE);
            readSize = recv(socketfd, recv_buf, BUFFER_SIZE, 0);
            if(readSize < 0){
                perror("Issue in reading the message");
                exit(1);
            }
        }
        char msgType[NAME_SIZE];
        msgType[0] = recv_buf[bytesRead];
        msgType[1] = recv_buf[bytesRead+1];
        msgType[2] = recv_buf[bytesRead+2];
        msgType[3] = recv_buf[bytesRead+3];
        msgType[4] = '\0';
        bytesRead = bytesRead + 4;
        //printf("asdasd%s\n", recv_buf);
        if(strcmp(msgType, "JOIN") == 0){
            printf("Total Message Size %d\n", readSize);
            // Extract name
            char name[NAME_SIZE];
            int count;
            int temp = bytesRead;
            for(count = bytesRead;count<readSize;count++){
                if(recv_buf[count] == '\r'){  
                    break;
                }
                name[count-temp] = recv_buf[count];
                bytesRead++;
            }
            bytesRead += 2;
            if(bytesRead != readSize){
                // printf("Read size %d\n", bytesRead);
                // printf("Message size %d\n", readSize);
                printf("Did not reach the end in JOIN\n");
                endOfMsg = 0;
            }
            else
            {
                endOfMsg = 1;
            }
            
            pthread_mutex_lock(&clients_mutex);
            for(int i=0; i<clientID;i++){
                Client* temp = queue[i];
                if(temp->socket == socketfd){
                    strcpy(temp->name, name);
                }
            }
            printf("New Connection Accepted from %s\n", name);
            pthread_mutex_unlock(&clients_mutex);
            printf("\n");   
        }

        else if(strcmp(msgType, "LIST") == 0){
            printf("\n");
            bytesRead += 2;
            if(bytesRead != readSize){
                // printf("Read size %d\n", bytesRead);
                // printf("Message size%d\n", readSize);
                printf("Did not reach the end in LIST\n");
                endOfMsg = 0;
            }
            else
            {
                printf("Reached the end in LIST\n");
                endOfMsg = 1;
            }
            char send_buf[BUFFER_SIZE];
            char name[NAME_SIZE];
            strcpy(send_buf, "LIST|");
            pthread_mutex_lock(&clients_mutex);
            for(int i =0;i<clientID;i++){
                Client* cli = queue[i];
                if(cli->socket == socketfd){
                    strcpy(name, cli->name);
                }
                Client* temp = queue[i];
                char temp_name[NAME_SIZE];
                strcpy(temp_name, temp->name);
                strcat(temp_name,"|");
                strcat(send_buf, temp_name);
            }
            strcat(send_buf,"\r\n");
            printf("Received LIST from %s\n", name);
            pthread_mutex_unlock(&clients_mutex);
            // printf("%d\n", socketfd);
            // printf("%d\n", queue[0]->socket);
            int sent = write(socketfd, send_buf, strlen(send_buf));
            if(sent < 0){
                printf("Write failed in LIST\n");
                exit(1);
            }
            printf("\n");
        }

        else if(strcmp(msgType, "UMSG") == 0){
            printf("\n");
            char send_buf[BUFFER_SIZE];
            int count = 0;
            char tname[NAME_SIZE];
            char ch = recv_buf[4];
            while(ch != '\r'){
                tname[count] = ch;
                count++;
                ch = recv_buf[4+count];
            }
            pthread_mutex_lock(&clients_mutex);
            int flag = 0;
            for(int i=0;i<clientID;i++){
                Client* cli = queue[i];
                if(strcmp(tname, cli->name) == 0){
                    // Send Message
                    flag = 1;
                    int sent = write(cli->socket, recv_buf, BUFFER_SIZE);
                    if(sent < 0){
                        printf("Write failed in USMG\n");
                        exit(1); 
                    }
                }
            }
            printf("Received UMSG from %s\n", tname);
            pthread_mutex_unlock(&clients_mutex);
            if(flag == 0){
                send_buf[0] = 'E';
                send_buf[1] = 'R';
                send_buf[2] = 'R';
                send_buf[3] = 'R';
                int count = 0;
                char err_msg[NAME_SIZE];
                strcpy(err_msg, "Name does not exist");
                char ch = err_msg[0];
                while(ch != '\0'){
                    send_buf[4+count] = ch;
                    count++;
                    ch = err_msg[count]; 
                }
                send_buf[4+count] = '\0';
                int sent = write(socketfd, send_buf, count+4);
                if(sent < 0){
                    printf("Write failed in USMG error part\n");
                    exit(1);
                }
            }
            printf("\n");
        }

        else if(strcmp(msgType, "BMSG") == 0){
            printf("\n");
            char send_buf[BUFFER_SIZE];            
            strcpy(send_buf, "BMSG");
            int count;
            int temp = bytesRead;
            char msg[BUFFER_SIZE];
            for(count = bytesRead;count<readSize;count++){
                if(recv_buf[count] == '\r')
                    break;
                msg[count-temp] = recv_buf[count];
                bytesRead++;
            }
            strcat(send_buf, msg);
            strcat(send_buf, "\r\n");
            bytesRead += 2;
            if(bytesRead != readSize){
                // printf("Read size %d\n", bytesRead);
                // printf("Message size %d\n", readSize);
                printf("Did not reach the end in BMSG\n");
                endOfMsg = 0;
            }
            else
            {
                printf("Reached the end in BMSG\n");
                endOfMsg = 1;
            }
            pthread_mutex_lock(&clients_mutex);
            char name[NAME_SIZE];
            for(int i =0;i<clientID;i++){
                Client* cli = queue[i];
                if(cli->socket == socketfd){
                    strcpy(name, cli->name);
                }
            }
            printf("Received BMSG from %s\n", name);
            for(int i = 0; i<clientID; i++){
                Client* cli = queue[i];
                if(write(cli->socket, send_buf, (int)strlen(send_buf)) < 0){
                    perror("Write failed in BMSG");
                    exit(1);
                }
                printf("Sent Message %s BMSG\n", send_buf);
            }
            pthread_mutex_unlock(&clients_mutex);
            printf("\n");
        }

        else if(strcmp(msgType, "LEAV") == 0){
            printf("\n");
            bytesRead += 2;
            if(bytesRead != readSize){
                endOfMsg = 0;
            }
            else
            {
                printf("Reached the end in LEAV\n");
                endOfMsg = 1;
            }
            pthread_mutex_lock(&clients_mutex);
            char name[NAME_SIZE];
            for(int i =0;i<clientID;i++){
                Client* cli = queue[i];
                if(cli->socket == socketfd){
                    strcpy(name, cli->name);
                }
            }
            printf("Received LEAV from %s\n", name);
            removeClient(socketfd);
            pthread_mutex_unlock(&clients_mutex);
            printf("\n");
        }
    }
}

void *handleClient(void* args){
    int connfd;
    connfd = *((int*)args);
    free(args);

    pthread_detach(pthread_self());
    strEcho(connfd);
    printf("asdasdasd\n");
    return(NULL);
}


int main(int argc, char*argv[]){
    
    struct sockaddr_in si_me;
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        printf("Socket did not initialise\n");
        exit(1);
    }
    int tempVal = bind(listenfd, (struct sockaddr*)&si_me, sizeof(si_me));
    if(tempVal < 0){
        printf("Binding error\n");
        exit(1);
    }
    printf("Binding successful\n");
    int temp1 = listen(listenfd, MAX_CLIENTS);
    if(temp1 < 0){
        printf("Error in listening\n");
        exit(1);
    }
    printf("Now listening on port %d..\n\n", PORT);
    bool server_running = true;
    pthread_t tid;

    while(server_running){
        struct sockaddr_in si_client;
        int len =  sizeof(si_client);
        int* cliSocket = (int*)malloc(sizeof(int));
        *cliSocket = accept(listenfd, (struct sockaddr*)&si_client, &len);
        
        // Client to the queue
        if(clientID+1 == MAX_CLIENTS){
            printf("Maximum clients reached\n");
            close(*cliSocket);
            continue;
        }
        Client* cli = (Client*)malloc(sizeof(Client));
        cli->si_other = si_client;
        cli->socket = *cliSocket;
        queue[clientID] = cli;
        clientID += 1;

        if(pthread_create(&tid, NULL, &handleClient, cliSocket) > 0){
            printf("Error while creating a pthread_t\n");
            exit(1);
        }
        // sleep(1);
        //pthread_join(tid, NULL);
    }
    return 0;
}