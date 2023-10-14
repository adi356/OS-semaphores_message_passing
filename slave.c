#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/sem.h>

struct sembuf sem_wait = {0, -1, 0}; // Decrement the semaphore
struct sembuf sem_signal = {0, 1, 0}; // Increment the semaphore

struct SharedData {
    int turn;
    int inCritical;
};

struct SharedData* sharedData;

void critical_section(int processNumber, FILE* logFile) {
    time_t t;
    time(&t);
    struct tm* currentTime = localtime(&t);

    // Format the time
    char message[50];
    strftime(message, sizeof(message), "%H:%M:%S", currentTime);

    // Write to cstest
    FILE* cstest = fopen("cstest", "a");
    fprintf(cstest, "%s File modified by process number %d\n", message, processNumber);
    fclose(cstest);
}

void entry_section(int processNumber, FILE* logFile) {
    fprintf(logFile, "Process %d entering critical section. \n", processNumber);
    sharedData[processNumber - 1].inCritical = 1;
}

void exit_section(int processNumber, FILE* logFile) {
    fprintf(logFile, "Process %d exiting critical section. \n", processNumber);
    sharedData[processNumber - 1].inCritical = 0;
}

int main(int argc, char* argv[]) {
    int processNumber = atoi(argv[1]);

    // Attach to shared memory
    key_t key = ftok("shared_memory_key", 1);
    int shm_id = shmget(key, sizeof(struct SharedData) * MAX_PROCESSES, 0666);
    sharedData = (struct SharedData*)shmat(shm_id, NULL, 0);

    // Create logFile
    char logFileName[20];
    snprintf(logFileName, sizeof(logFileName), "logfile.%d", processNumber);
    FILE* logFile = fopen(logFileName, "w");
    if (logFile == NULL) {
        perror("SLAVE ERROR: Problem opening log file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; ++i) {
        // Entry section
        semop(sem_id, &sem_wait, 1);
        entry_section(processNumber, logFile);

        sleep(rand() % 3 + 1);

        // Critical section
        critical_section(processNumber, logFile);

        sleep(rand() % 3 + 1);

        // Exit section
        exit_section(processNumber, logFile);
        semop(sem_id, &sem_signal, 1);
    }

    fclose(logFile);

    // Detach from shared memory
    shmdt(sharedData);

    return 0;
}

