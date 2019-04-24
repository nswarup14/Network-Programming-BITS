// Gen questions to be clarified
//  -> Should any buffer be flushed before new data comes?
//  -> Clarify with Sankalp on the logic
//  -> Port Number for every socket?
//  -> Zero Size DATA packet
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

#define RESEND 3
#define TIMEOUT 5
#define BUFFER_SIZE 516
#define MAX_CLIENTS 10

typedef enum 
{ 
    RRQ = 1, 
    DATA = 3,       
    ACK = 4,  
    ERR = 5,
    NONE = 6
}Opcode;

typedef struct _socket{
    int sock;
    struct _socket* next;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    int buffer_size;
    struct sockaddr_in si_other;
    clock_t sentAt;
    int port;
    int fileSize;
    char filepath[100];
    int resends;
    unsigned short int block_no;
    int endFileFlag;
    FILE* fp;
    clock_t irtt;
    
}Socket;

typedef struct _fd_queue{
    Socket* head;
    Socket* tail;
    int soc_count;
    int max_soc;
}SocketQueue;

typedef struct _error{
    unsigned short int opcode;
    unsigned short error_code;
    char errorMsg[30];
    int size;
}ErrPack;

typedef enum
{
    FILE_NOT_AVAILABLE = 0, 
    MAX_RESENDS

}ErrCode;

static ErrPack errs[] ={
    {1280, 256, "File not available", 18},
    {1280, 0, "Maximum resends", 15}
};

int socks[MAX_CLIENTS];
int no_clients = 0;

void sendPacket(Socket* temp_sock){
    // Create DATA packet to send
    if(temp_sock->fp == NULL){
        FILE* fp;
        fp = fopen("hello.txt", "rb");
        if(fp == NULL){
            printf("Error in opening the file\n");
            exit(1);
        }
        temp_sock->fp = fp;
    }
    temp_sock->block_no += 1;
    temp_sock->send_buffer[0] = 0;
    temp_sock->send_buffer[1] = DATA;
    temp_sock->send_buffer[2] = temp_sock->block_no >> 8;
    temp_sock->send_buffer[3] = temp_sock->block_no;

    int nread = fread(temp_sock->send_buffer+4, 1, 512, temp_sock->fp);
    temp_sock->buffer_size = strlen(temp_sock->send_buffer+4)+4;
    printf("Packet Size %d | ", temp_sock->buffer_size);
    int sent = sendto(temp_sock->sock, temp_sock->send_buffer, temp_sock->buffer_size, 0, (struct sockaddr*)&temp_sock->si_other, sizeof(temp_sock->si_other));
    if(sent < 0){
        printf("Error while sending DATA packet\n");
        exit(1);
    }
    if(nread < 512){
        temp_sock->endFileFlag = 1;
    }
    temp_sock->sentAt = clock();
}  

void resendPacket(Socket* temp_sock){
    int send = sendto(temp_sock->sock, temp_sock->send_buffer, sizeof(temp_sock->send_buffer), 0, (struct sockaddr*)&temp_sock->si_other, sizeof(temp_sock->si_other));
    if(send < 0){
        printf("Error while resending DATA packet\n");
        exit(1);
    }
    temp_sock->sentAt = clock();
}

void signal_handler(int signal){
    if(signal == SIGINT){
        printf("Server is closing\n");
        exit(1);
    }
}

void addSocket(Socket* node, SocketQueue* s_queue){
    Socket* head = s_queue->head;
    s_queue->tail = node;
    if(head == NULL){
        // Base case
        s_queue->head = node;
        s_queue->soc_count += 1;
        if(node->sock > s_queue->max_soc)
            s_queue->max_soc = node->sock;
        return;
    }
    while(head->next != NULL){
        head = head->next;
    }
    head->next = node;
    s_queue->soc_count += 1;
    if(node->sock > s_queue->max_soc)
        s_queue->max_soc = node->sock;
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

void removeSockNumber(int socketfd){
    for(int i =0;i<no_clients+1;i++){
        if(socks[i] == socketfd){
            for(int j = i;j<no_clients;j++){
                socks[j]=socks[j+1];
            }
            socks[no_clients] = -1;
            no_clients;
        }
    }
}

void extractName(char* listenfd_input, char* sub){
    int count = 2;
    char ch = listenfd_input[count];
    while(ch != '\0'){
        ch = listenfd_input[count];
        count++;
    }
    substring(listenfd_input, sub, 2, count-1);
}

void removeSocket(Socket* temp_sock, SocketQueue* s_queue){
    Socket* head = s_queue->head;
    Socket* prev;
    //HEAD case
    if(head->sock == temp_sock->sock){
        s_queue->head = head->next;
        head->next = NULL;
        free(head);
        s_queue->soc_count -= 1;
        removeSockNumber(temp_sock->sock);
        return;
    }
    while(head->sock != temp_sock->sock){
        prev = head;
        head = head->next;
    }
    if(head->next == NULL){//TAIL case
        s_queue->tail = prev;
    }
    prev->next = head->next;
    head->next = NULL;
    free(head);
    removeSockNumber(temp_sock->sock);
    s_queue->soc_count -= 1;
}

void pushQueue(SocketQueue* s_queue){
    Socket* head = s_queue->head;
    Socket* temp = s_queue->head;
    Socket* prev;
    if(temp->next == NULL){
        return;
    }
    while(temp->next != NULL){
        prev = temp;
        temp = temp->next;
    }
    prev->next = head;
    s_queue->head = head->next;
    s_queue->tail = head;
    head->next = NULL;
}

int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("To few arguments, read question\n");
        exit(1);
    }
    // Initiliase and Declare all necessary variables
    int port = atoi(argv[1]);
    struct sockaddr_in si_me, si_other;
    bool server_running = true;
    SocketQueue* s_queue = (SocketQueue*)malloc(sizeof(SocketQueue));
    s_queue->soc_count = 0;
    s_queue->head = NULL;
    s_queue->tail = NULL;
    s_queue->max_soc = -1;
    
    signal(SIGINT, signal_handler);

    // Setup the server
    printf("Setting up server...\n");
    int listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(listenfd < 0){
        printf("Socket did not initialise\n");
        exit(1);
    }
    char listenfd_input[BUFFER_SIZE];
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    int tempVal = bind(listenfd, (struct sockaddr*)&si_me, sizeof(si_me));
    if(tempVal < 0){
        printf("Binding error\n");
        exit(1);
    }
    printf("Listening on port %d..\n", port);
    // Set Timer
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    socks[no_clients] = listenfd;
    // Create FD list and add listening socket to it
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);
    s_queue->max_soc = listenfd;

    while(server_running){
        for(int i=0;i<no_clients+1;i++){
            // printf("Sockets:- %d\n", socks[i]);
            if(socks[i] != -1)
                FD_SET(socks[i], &readfds);
        }
        int response;
        if(s_queue->soc_count != 0){
            // printf("%d\n", s_queue->max_soc+1);
            response = select(s_queue->max_soc+1, &readfds, NULL, NULL, &timeout);
        }
        else
        {
            response = select(s_queue->max_soc+1, &readfds, NULL, NULL, NULL);
        }
        
        if(response < 0){
            printf("Select failed to deliver\n");
            exit(1);
        }
        // Timeout occurs-? Handle it differently for every single socket
        else if(response == 0){
            if (s_queue->soc_count > 0){   //Resend packet and update Timeout
                printf("Timeout while writing to %d\n", s_queue->head->sock);
                if(s_queue->head->resends++ == RESEND){
                    printf("Error %s\n", errs[1].errorMsg);
                    int sent = sendto(s_queue->head->sock, (void*)&errs[1], errs[1].size, 0, (struct sockaddr*)&s_queue->head->si_other, sizeof(s_queue->head->si_other));
                    if(send < 0){
                        printf("Error message sending failed\n");
                        exit(1);
                    }
                    removeSocket(s_queue->head, s_queue);
                    // Remove socket and stuff here
                }   
                else 
                    resendPacket(s_queue->head);
                    pushQueue(s_queue);
            }  
        }
        else if(response > 0){
            // Check for listenfd and s_queue->head->sock ONLY!!!
            if(FD_ISSET(listenfd, &readfds)){
                // Add new socket
                printf("\nA new connection has appeared\n");
                int slen = sizeof(si_other);
                int read_size = recvfrom(listenfd, listenfd_input, BUFFER_SIZE, 0, (struct sockaddr*)&si_other, &slen);
                if(read_size < 0){
                    printf("Error in receiving message\n");
                    exit(1);
                }
                no_clients += 1;
                int s_sock = socket(AF_INET, SOCK_DGRAM, 0);
                if(s_sock < 0){
                    printf(" Inner Socket did not initialise\n");
                    exit(1);
                }
                socks[no_clients] = s_sock;
                Socket* node = (Socket*)malloc(sizeof(Socket));
                node->si_other = si_other;
                node->port = port+no_clients;
                node->sock = s_sock;
                node->next = NULL;
                node->resends = 0;
                node->block_no = 0;
                node->endFileFlag = 0;
                node->fp = NULL;
                
                //Get Filename from RRQ
                char filename[BUFFER_SIZE];
                extractName(listenfd_input, filename);
                strcpy(node->filepath, filename);
                FILE* fp;
                printf("\nFile requested is %s\n", node->filepath);
                addSocket(node, s_queue);
                fp = fopen("hello.txt", "rb");
                if(fp == NULL) {
                    perror("Error is :");
                    printf("Error %s\n", errs[0].errorMsg);
                    int sent = sendto(node->sock, (void*)&errs[0], errs[0].size, 0, (struct sockaddr*)&node->si_other, sizeof(node->si_other));
                    if(send < 0){
                        printf("Error message sending failed\n");
                        exit(1);
                    }
                    exit(1);
                }
                fseek(fp,0,SEEK_END);
                node->fileSize = ftell(fp);
                fclose(fp);
                printf("FileSize is %d\n\n", node->fileSize);
                printf("Start Transferring\n");
                // Send first packet
                //sendPacket(node);
                //Close the streams and open stuff.
                memset(&si_other, 0, sizeof(si_other));
                memset(listenfd_input, 0, sizeof(listenfd_input));
            }
            if(FD_ISSET(s_queue->head->sock, &readfds)){
                int termFlag = 0;
                Socket* temp_sock = s_queue->head;
                int slen = sizeof(temp_sock->si_other);
                int read_size = recvfrom(temp_sock->sock, temp_sock->recv_buffer, BUFFER_SIZE, 0, (struct sockaddr*)&temp_sock->si_other, &slen);
                if(read_size < 0){
                    printf("Error in receiving message\n");
                    exit(1);
                }
                // Now check the respective opcodes
                if(temp_sock->recv_buffer[1] == ACK){
                    // Check if it is the right ACK number and then Send next packet
                    
                    //printf("%d\n", temp_sock->recv_buffer[2]);
                    // printf("%d\n", temp_sock->recv_buffer[3]);
                    unsigned short int block_no = (unsigned char)temp_sock->recv_buffer[3] + ((unsigned char)(temp_sock->recv_buffer[2]) << 8);
                    printf("Block No Received %d\n", block_no);
                    if(block_no == temp_sock->block_no){ //Right packet
                        memset(temp_sock->send_buffer, 0, sizeof(temp_sock->send_buffer));
                        if(temp_sock->endFileFlag){
                            termFlag = 1;
                            printf("Done Transferring....\n");
                            removeSocket(temp_sock, s_queue);
                        }
                        else{
                            sendPacket(temp_sock);
                            pushQueue(s_queue);
                        }
                    }
                    else{
                        resendPacket(temp_sock);
                        pushQueue(s_queue);
                    }
                }
                else if(temp_sock->recv_buffer[1] == ERR){
                    // Resend previous packet;
                    resendPacket(temp_sock);
                    pushQueue(s_queue);
                }
                else{
                    // Handle somehow? Not too sure
                }
                if(termFlag==0)
                    memset(temp_sock->recv_buffer, 0, sizeof(temp_sock->recv_buffer));
            }
        }
        // if(s_queue->soc_count != 0){

        //     timeout.tv_sec = TIMEOUT - (clock()-s_queue->head->sentAt)/CLOCKS_PER_SEC;
        //     // printf("%li\n", timeout.tv_sec);
        // }
    }
}