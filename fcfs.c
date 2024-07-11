/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#include "sch-helpers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <features.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#define _GNU_SOURCE


/*Simulated CPU Core*/
struct CPUCore{
    process_queue *readyQ;
    process_queue *DeviceQ;
    int totalCPUBusrt;
    bool cpuRunning = false;
    int *t;
    bool *systemIntialized;
}

struct Clock{
    int *t;
    bool *running;
}

struct ArrivalStruct{
    process_queue *q;
    int numberOfProcesses;
    int *t;
    process *pArray;
}

void* TimerThread(void*arg);
void* ArrivalThread(void*arg);
void* CPU(void*arg);
void* IO(void*arg);

int main(){

    /*get all the processes and sort them by arrival time*/
    process processes[MAX_PROCESSES+1];   
    int numberOfProcesses=0;  
    while (status=readProcess(&processes[numberOfProcesses]))  {
         if(status==1)  numberOfProcesses ++;
    } 
    qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

    /*Ready Queue for FCFS*/
    process_queue *readyQ = (process_queue *)malloc(sizeof(process_queue));    
    initializeProcessQueue(readyQ);
    /*Device I/O queue. Operating under the assumption all 4 processors share IO*/
    process_queue *DeviceQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(DeviceQ);

    int time=0;
    bool running = true;
    struct Clock *timer = (struct Clock *) malloc (sizeof(struct Clock));
    struct ArrivalStruct *arrivals = (struct ArrivalStruct *) malloc (sizeof(struct ArrivalStruct));
    timer->t = arrivals->t = &time;
    timer->running = &running;
    arrivals->pArray = & processes;
    arrivals->q=readyQ;
    arrivals->numberOfProcesses = numberOfProcesses;

    struct CPUCore * cpus = (struct CPUCore *) malloc(NUMBER_OF_PROCESSORS * sizeof(CPUCore));

}

void* TimerThread (void*arg){
    struct Clock* trueClock =(struct Clock *)arg;
    while(*(trueClock->running)){
        sleep(0.001);
        (*(trueClock->t))++;
    }
    return NULL;
}

void* ArrivalThread(void*arg){
    struct ArrivalStruct* arrived = (struct ArrivalStruct *)arg;
    int processNum=0;
    while(processNum < arrived->numberOfProcesses){
        if(*(arrived->t)>= arrived->pArray[processNum].arrivalTime){
            enqueueProcess(arrived->q,&(pArray[processNum]));
            processNum++;
        }
    }
    return NULL
}