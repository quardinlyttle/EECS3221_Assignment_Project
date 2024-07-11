/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#include "sch-helpers.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

struct CPUCore{
    process *currentProcess;
    int totalCPUBurst;
    bool cpuRunning;
    int nextReadyTime;
};

struct IO{
    process *currentProcess;
    int totalIOBurst;
    bool IORunning;
    int nextReadyTime;
};

struct Arrival{
    int arrival_number;
    process *arrivedProcess;
};

int main(){

    /*get all the processes and sort them by arrival time*/
    process processes[MAX_PROCESSES+1];   
    int numberOfProcesses=0;  
    int status;

    while ((status=readProcess(&processes[numberOfProcesses]))!=0)  {
         if(status==1)  numberOfProcesses ++;
    } 
    qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

    /*Ready Queue for FCFS*/
    process_queue *readyQ = (process_queue *)malloc(sizeof(process_queue));    
    initializeProcessQueue(readyQ);

    /*Device I/O queue. Operating under the assumption all 4 processors share IO*/
    process_queue *DeviceQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(DeviceQ);

    /*Completed Processes Queue*/
    process_queue *completedQ = (process_queue *)malloc(sizeof(process_queue));
    initializeProcessQueue(completedQ);

    /*Initialize CPUs*/
    struct CPUCore cpus[NUMBER_OF_PROCESSORS];
    for(int i=0;i<NUMBER_OF_PROCESSORS;i++){
        cpus[i].totalCPUBurst=0;
        cpus[i].cpuRunning=false;
        cpus[i].nextReadyTime=0;
    }

    /*Initialize IO*/
    struct IO io;
    io.totalIOBurst=0;
    io.IORunning=false;
    io.nextReadyTime=0;

    /*Initialize arrivals*/
    struct Arrival arrivals;
    arrivals.arrival_number=0;
    arrivals.arrivedProcess=&processs[0];


    /*Intitaize Time and running system*/
    int time = 0;
    bool systemRunning =  true;

    /*Each while loop will simulate one millisecond of time, assuming everything in this while loop is happeing in parrallel*/
    while(systemRunning){
        /*account for when all processes are done*/
        if(numberOfProcesses<arrivals.arrival_number)
        {
            systemRunning = false;
        }

        /*Adding processes to ready Q based on arrival time*/
        if(arrivals.arrivedProcess.arrivalTime >= time){
            enqueueProcess(readyQ,arrivals.arrivedProcess);
            printf("Process %d added to readyQ\n",arrivals.arrivedProcess.pid);
            arrivals.arrival_number++;
            arrivals.arrivedProcess=&processes[arrivals.arrival_number];
        }

        /*CPU Execution Simulation*/
        for (int i=0;i<NUMBER_OF_PROCESSORS;i++){
            /*Check if there is anything in the readyQ*/
            if(readyQ->size > 0){
                if(!(cups[i].cpuRunning)|| cpus[i].nextReadyTime >= time){
                    currentProcess = readyQ->front;
                    dequeueProcess(currentProcess);

                }

            }
        }

    }
}

