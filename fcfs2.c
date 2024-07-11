/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#define _GNU_SOURCE
#include "sch-helpers.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

/*Simulated CPU Core*/
struct CPUCore {
    process_queue *readyQ;
    process_queue *DeviceQ;
    process_queue *completedQ;
    int totalCPUBurst;
    bool cpuRunning;
    int *t;
    bool *systemInitialized;
    bool *systemRunning;
};

struct IO {
    process_queue *readyQ;
    process_queue *DeviceQ;
    process_queue *completedQ;
    int totalIOBurst;
    bool IORunning;
    int *t;
    bool *systemInitialized;
    bool *systemRunning;
};

struct Clock {
    int *t;
    bool *running;
};

struct ArrivalStruct {
    process_queue *q;
    int numberOfProcesses;
    int *t;
    process *pArray;
};

/* Function prototypes */
void TimerThread(struct Clock *timer);
void ArrivalThread(struct ArrivalStruct *arrivals);
void CPU(struct CPUCore *core);
void IOsim(struct IO *io);

int main() {
    /* Get all the processes and sort them by arrival time */
    process processes[MAX_PROCESSES + 1];
    int numberOfProcesses = 0;
    int status;

    while ((status = readProcess(&processes[numberOfProcesses])) != 0) {
        if (status == 1)
            numberOfProcesses++;
    }
    qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

    /* Ready Queue for FCFS */
    process_queue *readyQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(readyQ);

    /* Device I/O queue. Operating under the assumption all 4 processors share IO */
    process_queue *DeviceQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(DeviceQ);

    /* Completed Processes Queue */
    process_queue *completedQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(completedQ);

    int time = 0;
    bool running = true;
    struct Clock timer;
    struct ArrivalStruct arrivals;
    timer.t = arrivals.t = &time;
    timer.running = &running;
    arrivals.pArray = processes;
    arrivals.q = readyQ;
    arrivals.numberOfProcesses = numberOfProcesses;

    struct CPUCore *cpus = (struct CPUCore *)malloc(NUMBER_OF_PROCESSORS * sizeof(struct CPUCore));
    bool systemInitialized = false;

    struct IO *devIO = (struct IO *)malloc(sizeof(struct IO));

    /* Generate CPU Cores */
    for (int i = 0; i < NUMBER_OF_PROCESSORS; i++) {
        cpus[i].readyQ = readyQ;
        cpus[i].DeviceQ = DeviceQ;
        cpus[i].completedQ = completedQ;
        cpus[i].totalCPUBurst = 0;
        cpus[i].cpuRunning = false;
        cpus[i].t = &time;
        cpus[i].systemInitialized = &systemInitialized;
        cpus[i].systemRunning = &running;
    }

    /* Initialize and create IO, timer, and arrival processes */
    pid_t timerPid, arrivalPid, ioPid;

    timerPid = fork();
    if (timerPid == 0) {
        TimerThread(&timer);
        exit(0);
    } else if (timerPid < 0) {
        perror("Failed to fork timer process");
        return 1;
    }

    arrivalPid = fork();
    if (arrivalPid == 0) {
        ArrivalThread(&arrivals);
        exit(0);
    } else if (arrivalPid < 0) {
        perror("Failed to fork arrival process");
        return 1;
    }

    ioPid = fork();
    if (ioPid == 0) {
        IOsim(devIO);
        exit(0);
    } else if (ioPid < 0) {
        perror("Failed to fork IO process");
        return 1;
    }

    /* Simulate CPU Cores */
    systemInitialized = true;
    while (completedQ->size <= numberOfProcesses) {
        usleep(1000); // Sleep for 1 ms
    }
    running = false;

    // Wait for child processes to finish
    waitpid(timerPid, NULL, 0);
    waitpid(arrivalPid, NULL, 0);
    waitpid(ioPid, NULL, 0);

    /* Calculate statistics */
    int totalWaitTime = 0;
    int totalTurnAroundTime = 0;
    int totalCPU_Util = 0;

    for (int i = 0; i < numberOfProcesses; i++) {
        totalWaitTime += processes[i].waitingTime;
        totalTurnAroundTime += (processes[i].endTime - processes[i].startTime);
    }
    double avgWait = (double)totalWaitTime / numberOfProcesses;
    double avgTurnAround = (double)totalTurnAroundTime / numberOfProcesses;

    for (int i = 0; i < NUMBER_OF_PROCESSORS; i++) {
        totalCPU_Util += cpus[i].totalCPUBurst;
    }
    double avgCPU = ((double)totalCPU_Util / time) * 100.0 / NUMBER_OF_PROCESSORS;
    int lastpid = completedQ->back->data->pid;

    printf("Average waiting time:\t%f milliseconds\nAverage turnaround time:\t%f milliseconds\n", avgWait, avgTurnAround);
    printf("Time all processes finished:\t%d milliseconds\n", completedQ->back->data->endTime);
    printf("Average CPU Utilization:\t%f%%\nNumber of context switches:\t0\n", avgCPU);
    printf("PID of the last process to finish:\t%d\n", lastpid);

    /* Free Allocated Memory */
    free(readyQ);
    free(DeviceQ);
    free(completedQ);
    free(cpus);
    free(devIO);

    return 0;
}

/************************************************END OF MAIN******************************************/

/* Simulated Clock Thread */
void TimerThread(struct Clock *timer) {
    /* Generate simulated clock/timer */
    while (*(timer->running)) {
        usleep(1000); // Sleep for 1ms
        (*(timer->t))++;
    }
}

/* Simulated Processor arrival process */
void ArrivalThread(struct ArrivalStruct *arrivals) {
    int processNum = 0;
    /* Add processes to readyQ based on Arrival time */
    while (processNum < arrivals->numberOfProcesses) {
        if (*(arrivals->t) >= arrivals->pArray[processNum].arrivalTime) {
            enqueueProcess(arrivals->q, &(arrivals->pArray[processNum]));
            processNum++;
        } else {
            usleep(1000); // Sleep for 1ms
        }
    }
}

/* Simulated CPU process */
void CPU(struct CPUCore *core) {
    process *currentProcess;

    while (*(core->systemRunning)) {
        if (*(core->systemInitialized)) {
            if (core->readyQ->size > 0) {
                currentProcess = core->readyQ->front->data;
                dequeueProcess(core->readyQ);

                /* Populate Process data */
                if (currentProcess->startTime == 0) {
                    currentProcess->startTime = *(core->t);
                    currentProcess->waitingTime = currentProcess->startTime - currentProcess->arrivalTime;
                } else {
                    currentProcess->waitingTime += (*(core->t) - currentProcess->lastRequeTime);
                }

                /* Simulate CPU Burst */
                core->cpuRunning = true;
                usleep(currentProcess->bursts[currentProcess->currentBurst].length * 1000); // Simulate CPU burst
                core->cpuRunning = false;
                core->totalCPUBurst += currentProcess->bursts[currentProcess->currentBurst].length;

                /* Check if there is further IO. Move to appropriate Queue. */
                if (currentProcess->currentBurst == currentProcess->numberOfBursts) { // Process Complete
                    currentProcess->endTime = *(core->t);
                    enqueueProcess(core->completedQ, currentProcess);
                } else if ((currentProcess->currentBurst < currentProcess->numberOfBursts) && !(currentProcess->currentBurst % 2)) {
                    enqueueProcess(core->DeviceQ, currentProcess);
                    currentProcess->currentBurst++;
                } else {
                    printf("Process error for pid %d!!\n", currentProcess->pid);
                }
            } else {
                usleep(1000);
            }
        } else {
            usleep(1000);
        }
    }
}

/* IO Handler process. Assumes all 4 CPU Cores share IO */
void IOsim(struct IO *io) {
    process *currentProcess;

    while (*(io->systemRunning)) {
        if (*(io->systemInitialized)) {
            if (io->DeviceQ->size > 0) {
                currentProcess = io->DeviceQ->front->data;
                dequeueProcess(io->DeviceQ);

                /* Simulate IO Burst */
                io->IORunning = true;
                usleep(currentProcess->bursts[currentProcess->currentBurst].length * 1000); // Simulate IO burst
                io->IORunning = false;

                /* Check if there is further CPU. Move to appropriate Queue. */
                if (currentProcess->currentBurst == currentProcess->numberOfBursts) { // Process Complete
                    currentProcess->endTime = *(io->t);
                    enqueueProcess(io->completedQ, currentProcess);
                } else if ((currentProcess->currentBurst < currentProcess->numberOfBursts) && (currentProcess->currentBurst % 2)) {
                    currentProcess->lastRequeTime = *(io->t);
                    enqueueProcess(io->readyQ, currentProcess);
                    currentProcess->currentBurst++;
                } else {
                    printf("Process error for pid %d!!\n", currentProcess->pid);
                }
            } else {
                usleep(1000);
            }
        } else {
            usleep(1000);
        }
    }
}
