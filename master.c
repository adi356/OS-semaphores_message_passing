/**
 * @file master.c
 * @author Andrew Irvine (adi356@umsystem.edu)
 * @brief 
 * @version 0.1
 * @date 2023-09-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include "config.h"

struct SharedData {
    //turn variable to make sure each process has a unique turn
    int turn;
    int inCritical;
};

struct SharedData* sharedData;

void terminationFunc(int signo) {
    printf("Received termination signal. Cleaning up processes\n");

    //deallocate shared memory
    shmdt(sharedData);
    shmctl(shmget(ftok("shared_memory_key", 1), sizeof(struct SharedData) * MAX_PROCESSES, IPC_CREAT | 0666), IPC_RMID, NULL);
    
    //print termination time
    time_t t;
    time(&t);
    printf("Terminated at: %s", ctime(&t));

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
  
    //signal handlers
    signal(SIGTERM, terminationFunc);
    signal(SIGINT, terminationFunc);

    //timeout logic
    int timeout = atoi(argv[1]);
    int numProcesses = atoi(argv[2]);

    numProcesses = (numProcesses > MAX_PROCESSES) ? MAX_PROCESSES : numProcesses;

    //allocate shared memory and initialize
    key_t key = ftok("shared_memory_key", 1);
    int shmid = shmget(key, sizeof(struct SharedData) * MAX_PROCESSES, IPC_CREAT | 0666);
    sharedData = (struct SharedData*)shmat(shmid, NULL, 0);

    //initialize shared memory data for each process
    for (int i = 0; i < numProcesses; ++i) {
        sharedData[i].turn = i;
        sharedData[i].inCritical = 0;
    }

    //added to see if pid[i] would work better than pid_t
    // pid_t pid[MAX_PROCESSES];

    //create the multiple child processes using fork
    for (int i = 0; i < numProcesses; ++i) {
        pid_t pid = fork();
        //pid[i] = fork();
        if (pid == 0) {
            char processNumberStr[5];
            snprintf(processNumberStr, sizeof(processNumberStr), "%d", i + 1);
           execlp("./slave", "slave", processNumberStr, NULL);
           //TODO: write more descriptive error
           perror("execlp");
           exit(EXIT_FAILURE);
        } else if (pid < 0) {
            //TODO: write more descriptive error
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    sleep(timeout);

    //added to test new method for this loop
    
    //terminate child processes
    //should have been inside termination function along with kill operation below
    for (int i = 0; i < numProcesses; ++i) {
        kill(0, SIGTERM);
        //usleep(5000); //delay between signals to each child process
    }

    //wait for child processes to terminate
    //this should be above the for loop killing child processes and also inside termination function
    int status;
    while (wait(&status) > 0);

    //deallocate shared memory
    shmdt(sharedData);
    shmctl(shmget(key, sizeof(struct SharedData) * MAX_PROCESSES, IPC_CREAT | 0666), IPC_RMID, NULL);
    //shmctl(shmid, IPC_RMID, NULL);

    //print time
    time_t t;
    time(&t);
    printf("Terminated at: %s\n", ctime(&t));

    return 0;
}
