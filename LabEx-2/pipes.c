#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

void removeLastLetter(char* name){
    int length;
    name[length-1] = '\0';
    return;
}

char* removeFirstLetter(char* name){
    char* newName;
    newName = name + 1;
    return newName;
}

char* convertLowerToUpper(char* name){
    int c = 0;
    while(name[c] != '\0'){
        if(name[c] >= 'a' && name[c] <= 'z'){
            name[c] = name[c] - 32;
        }
        c++;
    }
    return name;
}

int main(){

    pid_t mainProcess = getpid();
    int numOfChildProcesses = 5;
    pid_t processIds[5];
    pid_t childProcess;
    char* buf = (char*)malloc(sizeof(char)*100);

    // Create Pipes here
    int fd[6][2];
    for(int i=0; i<(numOfChildProcesses+1); i++){
        pipe(fd[i]);
    }
    
    int localPid = 0;
    for(int i=0; i<(numOfChildProcesses); i++){
        localPid++;
        if((childProcess=fork()) == 0){
            // Child Process
            char* tempBuf;
            close(fd[i][1]);
            read(fd[i][0], tempBuf, 100);
            switch (localPid)
            {
                case 1:
                    tempBuf = convertLowerToUpper(tempBuf);
                    break;
                case 2:
                    tempBuf = removeFirstLetter(tempBuf);
                    break;
                case 3:
                    removeLastLetter(tempBuf);
                    break;
                case 4:
                    tempBuf = removeFirstLetter(tempBuf);
                    break;
                case 5:
                    removeLastLetter(tempBuf);
                    break;
                default:
                    printf("Error in some process %d\n", getpid());
                    break;
            }
            write(fd[i+1][1], tempBuf, 100);
            printf("Child Process Pid %d; String Output %s\n", getpid(), tempBuf);
            break;
        }
        else{

        }
    }
    if(getpid() == mainProcess){
        close(fd[0][0]);
        printf("Enter something please \n");
        fgets(buf, 100, stdin);
        write(fd[0][1], buf, 100);
        read(fd[5][0], buf, 100);
        printf("Main Process Pid %d; String Output %s\n", getpid(), buf);
    }
    return 0;
}