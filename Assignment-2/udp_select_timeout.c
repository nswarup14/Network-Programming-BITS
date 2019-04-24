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

void sendEmptyData(Socket* temp_sock){
    temp_sock->send_buffer[0] = 0;
    temp_sock->send_buffer[1] = DATA;
    temp_sock->send_buffer[2] = temp_sock->block_no >> 8;
    temp_sock->send_buffer[3] = temp_sock->block_no;
    temp_sock->buffer_size = sizeof(temp_sock->send_buffer);
    int sent = sendto(temp_sock->sock, temp_sock->send_buffer, temp_sock->buffer_size, 0, (struct sockaddr*)&temp_sock->si_other, sizeof(temp_sock->si_other));
    if(sent < 0){
        printf("Error while sending DATA packet\n");
        exit(1);
    }
    temp_sock->sentAt = clock();
}

void sendPacket(Socket* temp_sock){
    FILE* fp;
    fp = fopen(temp_sock->filepath, "rb");
    if(fp == NULL){
        printf("Error in opening the file\n");
        exit(1);
    }
    // Create DATA packet to send
    temp_sock->send_buffer[0] = 0;
    temp_sock->send_buffer[1] = DATA;
    temp_sock->send_buffer[2] = temp_sock->block_no >> 8;
    temp_sock->send_buffer[3] = temp_sock->block_no;

    int endFlag = 0;
    int count = 0;
    temp_sock->block_no += 1;
    fread(temp_sock->send_buffer+4, 1, 512, fp);
    temp_sock->buffer_size = sizeof(temp_sock->send_buffer);
    int sent = sendto(temp_sock->sock, temp_sock->send_buffer, temp_sock->buffer_size, 0, (struct sockaddr*)&temp_sock->si_other, sizeof(temp_sock->si_other));
    if(sent < 0){
        printf("Error while sending DATA packet\n");
        exit(1);
    }
    temp_sock->endFileFlag = 1;
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

void extractName(char* listenfd_input, char* sub){
    int count = 2;
    char ch = listenfd_input[count];
    while(ch != '\0'){
        ch = listenfd_input[++count];
    }
    substring(listenfd_input, sub, 2, count-2);
}

void removeSocket(Socket* temp_sock, SocketQueue* s_queue){
    Socket* head = s_queue->head;
    Socket* prev;
    //HEAD case
    if(head->sock == temp_sock->sock){
        s_queue->head = head->next;
        head->next = NULL;
        free(head);
        return;
    }
    while(head->sock != temp_sock->sock){
        prev = head;
        head = head->next;
    }
    if(head->next == NULL){ //TAIL case
        s_queue->tail = prev;
    }
    prev->next = head->next;
    head->next = NULL;
    free(head);
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
    int no_clients = 0;
    char basepath[BUFFER_SIZE];
    strcpy(basepath, "/srv/tftp/");
    SocketQueue* s_queue = (SocketQueue*)malloc(sizeof(SocketQueue));
    s_queue->soc_count = 0;
    s_queue->head = NULL;
    s_queue->tail = NULL;
    s_queue->max_soc = -1;
    
    //signal(SIGINT, signal_handler);

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
    int* socks = (int*)malloc(sizeof(int)*(MAX_CLIENTS+1));
    socks[no_clients] = listenfd;
    // Create FD list and add listening socket to it
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    while(server_running){
        for(int i=0;i<no_clients;i++){
            FD_SET(socks[i], &readfds);
        }
        int response = select(s_queue->max_soc, &readfds, NULL, NULL, &timeout);
        if(response < 0){
            printf("Select failed to deliver\n");
            exit(1);
        }
        // Timeout occurs-? Handle it differently for every single socket
        if(response == 0){
            printf("Timeout while reading from %d\n", s_queue->head->sock);
            if(s_queue->soc_count == 0){
                continue;
            }
            else if (s_queue->soc_count > 0)
            {   //Resend packet and update Timeout
                if(s_queue->head->resends++ == RESEND){
                    printf("Error %s\n", errs[1].errorMsg);
                    int sent = sendto(s_queue->head->sock, (void*)&errs[1], errs[1].size, 0, (struct sockaddr*)&s_queue->head->si_other, sizeof(s_queue->head->si_other));
                    if(send < 0){
                        printf("Error message sending failed\n");
                        exit(1);
                    }
                }   
                else 
                    resendPacket(s_queue->head);
            }  
        }
        else if(response > 0){
            // Check for listenfd and s_queue->head->sock ONLY!!!
            if(FD_ISSET(listenfd, &readfds)){
                // Add new socket
                int slen = sizeof(si_other);
                int read_size = recvfrom(listenfd, listenfd_input, BUFFER_SIZE, 0, (struct sockaddr*)&si_other, &slen);
                if(read_size < 0){
                    printf("Error in receiving message\n");
                    exit(1);
                }
                no_clients += 1;
                int s_sock = socket(AF_INET, SOCK_DGRAM, 0); 
                socks[no_clients] = s_sock;
                Socket* node = (Socket*)malloc(sizeof(Socket));
                node->si_other = si_other;
                node->port = port+no_clients;
                node->sock = s_sock;
                node->next = NULL;
                node->resends = 0;
                node->block_no = 0;
                node->endFileFlag = 0;
                addSocket(node, s_queue);
                
                //Get Filename from RRQ
                char filename[BUFFER_SIZE];
                char sub[BUFFER_SIZE];
                extractName(listenfd_input, sub);
                strcpy(filename, sub);
                char filepath[100];
                strcpy(filepath, basepath);
                strcat(filepath, filename);
                strcpy(node->filepath, filepath);
                FILE* fp;
                printf("Filepath is at %s\n", node->filepath);
                fp = fopen(filepath, "rb");
                if(fp == NULL){
                    printf("Error %s\n", errs[0].errorMsg);
                    int sent = sendto(node->sock, (void*)&errs[0], errs[0].size, 0, (struct sockaddr*)&node->si_other, sizeof(node->si_other));
                    if(send < 0){
                        printf("Error message sending failed\n");
                        exit(1);
                    }

                }
                fseek(fp,0,SEEK_END);
                node->fileSize = ftell(fp);
                
                // Send first packet
                sendPacket(node);

                //Close the streams and open stuff.
                fclose(fp);
                memset(&si_other, 0, sizeof(si_other));
                memset(listenfd_input, 0, sizeof(listenfd_input));
            }
            if(FD_ISSET(s_queue->head->sock, &readfds)){
                Socket* temp_sock = s_queue->head;
                int slen = sizeof(temp_sock->si_other);
                int read_size = recvfrom(temp_sock->sock, temp_sock->recv_buffer, BUFFER_SIZE, 0, (struct sockaddr*)&temp_sock->si_other, &slen);
                if(read_size < 0){
                    printf("Error in receiving message\n");
                    exit(1);
                }
                if(temp_sock->recv_buffer[0]){
                    temp_sock->recv_buffer[1] = NONE;
                }

                // Now check the respective opcodes
                if(temp_sock->recv_buffer[1] == ACK){
                    // Check if it is the right ACK number and then Send next packet
                    int termFlag = 0;
                    unsigned short int block_no = (unsigned char)temp_sock->recv_buffer[3] + (unsigned char)(temp_sock->recv_buffer[2]) << 8;
                    if(block_no == temp_sock->block_no){ //Right packet
                        memset(temp_sock->send_buffer, 0, sizeof(temp_sock->send_buffer));
                        if(temp_sock->endFileFlag){
                            if(temp_sock->fileSize%512 == 0){
                                sendEmptyData(temp_sock);
                            }
                            termFlag = 1;
                        }
                        else{
                            sendPacket(temp_sock);
                        }
                    }
                    else{
                        resendPacket(temp_sock);
                    }
                    if(termFlag){
                        printf("Done Transferring....\n");
                        removeSocket(temp_sock, s_queue);
                    }
                }
                else if(temp_sock->recv_buffer[1] == ERR){
                    // Resend previous packet;
                    resendPacket(temp_sock);
                }
                else{
                    // Handle somehow? Not too sure
                }
                memset(temp_sock->recv_buffer, 0, sizeof(temp_sock->recv_buffer));
            }
        }
        timeout.tv_sec = 5.0 - (clock()-s_queue->head->sentAt);
    }
}