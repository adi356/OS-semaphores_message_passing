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


struct SharedData {
    int turn;
    int inCritical;
};

struct SharedData* sharedData;

void critical_section(int processNumber, FILE* logFile) {
    //create message format HH:MM:SS
    time_t t;
    time(&t);
    char message[50];
    strftime(message, sizeof(message), "%H:%M:%S", localtime(&t));
    fprintf(logFile, "%s Process %d entered critical section. \n", message, processNumber);

    //write to cstest and append
    FILE* cstest = fopen("cstest", "a");
    fprintf(cstest, "%s File modified by process number %d\n", message, processNumber);
    fclose(cstest);
}

void entry_section (int processNumber, FILE* logFile) {
    fprintf(logFile, "Process %d entering critical section. \n", processNumber);
    sharedData[processNumber - 1].inCritical = 1;
}

void exit_section (int processNumber, FILE* logFile) {
    fprintf(logFile, "Process %d exiting critical section. \n", processNumber);
    sharedData[processNumber - 1].inCritical = 0;
}

// void random_sleep() {
//     //sleep for random amount of time between 1 and 3 seconds
//     sleep(rand() % 3 + 1);
// }

int main(int argc, char* argv[]) {
    int processNumber = atoi(argv[1]);

    key_t key = ftok("shared_memory_key", 1);
    int shmid = shmget(key, sizeof(struct SharedData) * MAX_PROCESSES, 0666);
    sharedData = (struct SharedData*)shmat(shmid, NULL, 0);

    //open logFile
    char logFileName[20];
    snprintf(logFileName, sizeof(logFileName), "logfile.%d", processNumber);
    FILE* logFile = fopen(logFileName, "w");
    if (logFile == NULL) {
        //should use printf here instead of perror
        perror("SLAVE ERROR: Problem opening log file");
        exit(EXIT_FAILURE);
    }

    //loop for critical section
    for (int i = 0; i < 5; ++i) {
        
        entry_section(processNumber, logFile);
        sleep(rand() % 3 + 1);
        critical_section(processNumber, logFile);
        sleep(rand() % 3 + 1);
        exit_section(processNumber, logFile);
        //remainder_section();
    }

    //close logFile
    fclose(logFile);

    //Detach from shared memory
    shmdt(sharedData);

    return 0;
}
