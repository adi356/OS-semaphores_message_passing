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
#include <sys/sem.h>
#include "config.h"

struct sembuf sem_wait = {0, -1, 0}; // Decrement the semaphore
struct sembuf sem_signal = {0, 1, 0}; // Increment the semaphore


struct SharedData {
    int turn;
    int inCritical;
};

struct SharedData* sharedData;

void terminationFunc(int signo) {
    printf("Received termination signal. Cleaning up processes\n");

    // Terminate child processes
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        kill(0, SIGTERM);
    }

    // Wait for child processes to terminate
    int status;
    while (wait(&status) > 0);

    // Deallocate semaphore
    semctl(sem_id, 0, IPC_RMID);

    // Deallocate shared memory
    shmdt(sharedData);
    shmctl(shm_id, IPC_RMID, NULL);

    // Print termination time
    time_t t;
    time(&t);
    printf("Terminated at: %s", ctime(&t));

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    // Signal handlers
    signal(SIGTERM, terminationFunc);
    signal(SIGINT, terminationFunc);

    //original
    // int timeout = (argc > 1) ? atoi(argv[1]) : DEFAULT_TIMEOUT;
    // int numProcesses = (argc > 2) ? atoi(argv[2]) : MAX_PROCESSES;
    // numProcesses = (numProcesses > MAX_PROCESSES) ? MAX_PROCESSES : numProcesses;

    //new 
    int timeout = (argc > 2) ? atoi(argv[2]) : DEFAULT_TIMEOUT;
    int numProcesses = (argc > 3) ? atoi(argv[3]) : MAX_PROCESSES;
    numProcesses = (numProcesses > MAX_PROCESSES) ? MAX_PROCESSES : numProcesses;

    // Create semaphore
    sem_id = semget(ftok("semaphore_key", 1), 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("master: Error creating semaphore");
        exit(EXIT_FAILURE);
    } else {
        printf("Semaphore created successfully. ID: %d\n", sem_id);
    }

    // Initialize semaphore value to numProcesses
    semctl(sem_id, 0, SETVAL, numProcesses);

    // Allocate shared memory and initialize
    key_t key = ftok("shared_memory_key", 1);
    shm_id = shmget(key, sizeof(struct SharedData) * MAX_PROCESSES, IPC_CREAT | 0666);
    sharedData = (struct SharedData*)shmat(shm_id, NULL, 0);

    // Initialize shared memory data for each process
    for (int i = 0; i < numProcesses; ++i) {
        sharedData[i].turn = i;
        sharedData[i].inCritical = 0;
    }

    // Create the multiple child processes using fork
    for (int i = 0; i < numProcesses && i < MAX_PROCESSES; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            char processNumberStr[5];
            snprintf(processNumberStr, sizeof(processNumberStr), "%d", i + 1);
            execlp("./slave", "slave", processNumberStr, NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Loop to wait for child processes to terminate
    int status;
    for (int i = 0; i < numProcesses; ++i) {
        wait(&status);
    }

    // Deallocate semaphore and shared memory
    terminationFunc(0);

    return 0;
}