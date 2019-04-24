// P_Threads
// Compiling gcc pthread.c -lpthread
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
    char recv_buf[BUFFER_SIZE];
    while((readSize = read(socketfd, recv_buf, BUFFER_SIZE)) > 0){
        char msgType[5];
        msgType[0] = recv_buf[0];
        msgType[1] = recv_buf[1];
        msgType[2] = recv_buf[2];
        msgType[3] = recv_buf[3];
        msgType[4] = '\0';

        if(strcmp(msgType, "JOIN") == 0){
            // Extract name
            char name[NAME_SIZE];
            char ch = recv_buf[4];
            int count = 0;
            while(ch != '\r'){
                name[count] = ch;
                count++;
                ch = recv_buf[4+count];
            }
            pthread_mutex_trylock(&clients_mutex);
            for(int i=0; i<clientID;i++){
                Client* temp = queue[i];
                if(temp->socket == socketfd){
                    strcpy(temp->name, name);
                }
            }
            pthread_mutex_unlock(&clients_mutex);   
        }
        else if(strcmp(msgType, "LIST") == 0){
            char send_buf[BUFFER_SIZE];
            send_buf[0] = 'L';
            send_buf[1] = 'I';
            send_buf[2] = 'S';
            send_buf[3] = 'T';
            int count = 4;
            pthread_mutex_trylock(&clients_mutex);
            for(int i=0;i<clientID; i++){
                Client* temp = queue[i];
                char temp_name[BUFFER_SIZE];
                strcpy(temp_name, temp->name);
                char ch = temp_name[0];
                int int_count = 0;
                while(ch != '\0'){
                    send_buf[count] = ch;
                    int_count++;
                    count++;
                    ch = temp_name[int_count];
                }
                send_buf[count] = '\0';
                count++;
            }
            pthread_mutex_unlock(&clients_mutex);
            int sent = write(socketfd, send_buf, count);
            if(sent < 0){
                printf("Write failed in LIST\n");
                exit(1);
            }
        }
        else if(strcmp(msgType, "UMSG") == 0){
            char send_buf[BUFFER_SIZE];
            int count = 0;
            char tname[NAME_SIZE];
            char ch = recv_buf[4];
            while(ch != '\r'){
                tname[count] = ch;
                count++;
                ch = recv_buf[4+count];
            }
            pthread_mutex_trylock(&clients_mutex);
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
        }
        else if(strcmp(msgType, "BMSG") == 0){
            pthread_mutex_lock(&clients_mutex);
            for(int i = 0; i<clientID; i++){
                Client* cli = queue[i];
                if(cli->socket == socketfd){
                    continue;
                }
                if(write(cli->socket, recv_buf, BUFFER_SIZE) < 0){
                    printf("Write failed in BSMG\n");
                    exit(1);
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
        else if(strcmp(msgType, "LEAV") == 0){
            pthread_mutex_trylock(&clients_mutex);
            removeClient(socketfd);
            pthread_mutex_unlock(&clients_mutex);
        }
    }
}

void *handleClient(void* args){
    int connfd;
    connfd = *((int*)args);
    free(args);

    pthread_detach(pthread_self());
    strEcho(connfd);
    
    close(connfd);
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
    printf("Now listening on port %d..\n", PORT);
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

        printf("New Connection Accepted\n");
        if(pthread_create(&tid, NULL, &handleClient, cliSocket) > 0){
            printf("Error while creating a pthread_t\n");
            exit(1);
        }
        sleep(1);
        pthread_join(tid, NULL);
    }
    return 0;
}