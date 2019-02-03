#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int global_count = 0;

void handlerFunction(int signo){
    global_count++;
    printf("The process %d recieved the following signal %d\n", getpid(), signo);
}

void sendSignal(pid_t pid, int signal_no){
    kill(pid, signal_no);
}

int selectProcess(int lineNumber){
    //Check if line number is within the limit of the file
    FILE *ptr = fopen("pids.txt", "r+");
    if(ptr == NULL){
        printf("Cannot open File\n");
        exit(1);
    }
    int count = 1;
    char line[5];
    while(fgets(line, sizeof(line), ptr) != NULL){
        if(lineNumber == count)
        {
            fclose(ptr);
            return atoi(line);
        }
        else
            count++;
    }
}

int main(int argc, char *argv[]){
    if(argc < 5){
        printf("To few arguments, read question\n");
        exit(1);
    }
    pid_t pid;
    int i,j; 
    int N = atoi(argv[1]);
    int K = atoi(argv[2]);
    int L = atoi(argv[3]);
    int M = atoi(argv[4]);
    int total_processes = 1+N+N*K;
    FILE *fptr = fopen("pids.txt", "w+");
    if(fptr == NULL)
    {
        printf("Unable to open file\n");
        exit(1);
    }

    // Assign handler and set the global count of recieved signals [Can be RESET]
    for(j=1;j<32;j++){
        signal(j, handlerFunction);
    }
    pid = getpid();
    fprintf(fptr, "%d\n", pid);
    fclose(fptr);
    pid_t mainProcess = pid;
    pid_t *allPids = (pid_t*)malloc(sizeof(pid_t)*(1+N+N*K)); // How to populate this?
    int childrenSize = N >= K? N:K;
    pid_t *childrenPids = (pid_t*)malloc(sizeof(pid_t)*childrenSize);
    int childCount = 0;

    pid_t id1, id2;
    while(N-- > 0){
        if((id1 = fork())==0){
            while(K-- > 0){
                //printf("N and K: %d %d\n", N, K);
                if((id2 = fork())==0){
                    childCount = 0;
                    break;
                }
                else{
                    fptr = fopen("pids.txt", "a+");
                        if(fptr == NULL){
                            printf("Unable to open file\n");
                            exit(1);
                        }
                    fprintf(fptr, "%d\n", id2);
                    //printf("In child of child %d\n", id2);
                    fclose(fptr);
                    childrenPids[childCount++] = id2;
                }
            }
            pause();
            break;
        }
        else{
            fptr = fopen("pids.txt", "a+");
                if(fptr == NULL){
                    printf("Unable to open file\n");
                    exit(1);
                }
            fprintf(fptr, "%d\n", id1);
            //printf("In child %d\n", id1);
            fclose(fptr);
            childrenPids[childCount++] = id1;
        }
    }
    sleep(4);
    // Send a wake up signal to every other process.
    if(getpid() == mainProcess){
        printf("Main process just woke up\n");
        kill(0, SIGUSR1);
    }
    srand(getpid()); 
    // Check if all the processes have written their PID's on the textfile. [DONE]
    
    int tempM = M;
    int rand_signal;
    int rand_line;
    while(1){
        while(M-- > 0){
            rand_signal = rand()%31 + 1;
            rand_line = rand()%total_processes + 1;
            
            pid_t awaiting_process = selectProcess(rand_line); 
            sendSignal(awaiting_process, rand_signal);
        }
        if(global_count >= L){
            printf("Process %d received %d number of signals. So terminating\n", getpid(), global_count);
            exit(0);
        }
        M = tempM;
    }

}
