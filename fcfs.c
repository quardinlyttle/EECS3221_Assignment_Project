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
         printf("Process %d added to array\n",processes[numberOfProcesses-1].pid);
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

    printf("Queues intialized\n");

    /*Initialize CPUs*/
    struct CPUCore cpus[NUMBER_OF_PROCESSORS];
    for(int i=0;i<NUMBER_OF_PROCESSORS;i++){
        cpus[i].totalCPUBurst=0;
        cpus[i].cpuRunning=false;
        cpus[i].nextReadyTime=0;
        cpus[i].currentProcess=NULL;
    }

    /*Initialize IO*/
    struct IO io;
    io.totalIOBurst=0;
    io.IORunning=false;
    io.nextReadyTime=0;
    io.currentProcess=NULL;

    /*Initialize arrivals*/
    struct Arrival arrivals;
    arrivals.arrival_number=0;
    arrivals.arrivedProcess=NULL;
    if(numberOfProcesses>0){
        arrivals.arrivedProcess=&processes[0];
    }


    /*Intitaize Time and running system*/
    int time = 0;
    bool systemRunning =  true;

    /*Each while loop will simulate one millisecond of time, assuming everything in this while loop is happeing in parrallel*/
    while(systemRunning){
        printf("Timestamp: %d\n",time);

        /*account for when all processes are done*/
        if(numberOfProcesses<=arrivals.arrival_number)
        {
            systemRunning = false;
        }

        /*Adding processes to ready Q based on arrival time*/
        if(arrivals.arrivedProcess != NULL && arrivals.arrivedProcess->arrivalTime >= time){
            enqueueProcess(readyQ,arrivals.arrivedProcess);
            printf("Process %d added to readyQ\n",arrivals.arrivedProcess->pid);
            arrivals.arrival_number++;
            if (arrivals.arrival_number < numberOfProcesses) {
                arrivals.arrivedProcess = &processes[arrivals.arrival_number];
            } else {
                arrivals.arrivedProcess = NULL;
            }
        }

        /*Checking if IO is running*/
        if(io.nextReadyTime < time)
        {
            io.IORunning=true;
        }
        else{
            io.IORunning=false;
        }

        if(!(io.IORunning)){
            if(io.currentProcess!=NULL){
                io.currentProcess->currentBurst++;
                enqueueProcess(readyQ,io.currentProcess);
                printf("Process %d added to ReadyQ",io.currentProcess->pid);
            }
            
            /*IO checks if anthing in deviceQ*/
            if(DeviceQ->size > 0){
                io.currentProcess=DeviceQ->front->data;
                dequeueProcess(DeviceQ);
                io.nextReadyTime = time + io.currentProcess->bursts[io.currentProcess->currentBurst].length;
            }

        }

        /*CPU Execution Simulation*/
        for (int i=0;i<NUMBER_OF_PROCESSORS;i++){

            /*Check which CPU is not busy*/
            if(cpus[i].nextReadyTime > time){
                cpus[i].cpuRunning=true;
                printf("CPU %d is busy and will be free at %d\n",i,cpus[i].nextReadyTime);
            }
            else{
                cpus[i].cpuRunning=false;
                printf("CPU %d is free at %d\n",i,cpus[i].nextReadyTime);
            }

            /*Runs if the CPU is free*/
            if(cpus[i].cpuRunning==false){
                printf("CPU RUnning var= %d",cpus[i].cpuRunning);
                printf("CPU %d is running\n",i);

                /*Check if there was a process and move to IO since its done.*/
                if(cpus[i].currentProcess != NULL && (cpus[i].currentProcess->currentBurst < cpus[i].currentProcess->numberOfBursts)){
                    enqueueProcess(DeviceQ,cpus[i].currentProcess);
                    printf("Process %d added to DeviceQ\n",cpus[i].currentProcess->pid);
                }
                /*Add process to the list of completed Processes*/
                else if(cpus[i].currentProcess != NULL && (cpus[i].currentProcess->currentBurst >= cpus[i].currentProcess->numberOfBursts)) {
                    printf("ending process %d\n",cpus[i].currentProcess->pid);
                    //cpus[i].currentProcess->endTime =time;
                    enqueueProcess(completedQ,cpus[i].currentProcess);
                }

                /*Check if there is anything in the readyQ*/
                if(readyQ->size > 0){
                    /*take new process from ReadyQ*/
                    cpus[i].currentProcess = readyQ->front->data;
                    dequeueProcess(readyQ);

                    /*Determine start time and add wait time*/
                    if(cpus[i].currentProcess->startTime == 0){
                        cpus[i].currentProcess->startTime = time;
                        cpus[i].currentProcess->waitingTime = cpus[i].currentProcess->startTime - cpus[i].currentProcess->arrivalTime;
                        } 
                    else{
                        cpus[i].currentProcess->waitingTime += (time-cpus[i].currentProcess->lastRequeTime);
                    }
                    
                    cpus[i].cpuRunning=true;
                    cpus[i].totalCPUBurst+=cpus[i].currentProcess->bursts[cpus[i].currentProcess->currentBurst].length;
                    /*Initialize currentBurst if not already done so*/
                    if(cpus[i].currentProcess->currentBurst==-1){
                        cpus[i].currentProcess->currentBurst=0;
                    }
                    /*identify*/
                    if (cpus[i].currentProcess->currentBurst < cpus[i].currentProcess->numberOfBursts){
                        cpus[i].nextReadyTime = time + cpus[i].currentProcess->bursts[cpus[i].currentProcess->currentBurst].length;
                        cpus[i].currentProcess->currentBurst++;
                    }
                    printf("CPU %d will be available at timestamp: %d\n",i,cpus[i].nextReadyTime);
                    
                }
                else{
                    printf("CPU %d is free\n",i);
                    cpus[i].cpuRunning=false;
                }

            }
        }

        time++;

    }

    int totalWaitTime=0;
    int totalTurnAroundTime=0;
    int totalCPU_Util=0;

    for (int i=0; i<numberOfProcesses; i++){
        totalWaitTime += processes[i].waitingTime;
        totalTurnAroundTime += (processes[i].endTime-processes[i].startTime);
    }
    double avgWait = (double)totalWaitTime/numberOfProcesses;
    double avgTurnAround = (double)totalTurnAroundTime/numberOfProcesses;

    for (int i =0; i< NUMBER_OF_PROCESSORS; i++){
        totalCPU_Util += (cpus[i].totalCPUBurst);
    }
    double avgCPU= ((double) totalCPU_Util / time) * 100.0 / NUMBER_OF_PROCESSORS;
    int lastpid = completedQ->back->data->pid;

    printf("Average waiting time:\t%f milliseconds\nAverage turnaround time:\t%f milliseconds\n",avgWait,avgTurnAround);
    printf("Time all processes finished:\t%d milliseconds\n",completedQ->back->data->endTime);
    printf("Average CPU Utilization:\t%f%%\nNumber of context switches:\t0\n",avgCPU);
    printf("PID of the last process to finish:\t%d\n",lastpid);
    
    free(readyQ);
    free(DeviceQ);
    free(completedQ);

    return 0;

}

