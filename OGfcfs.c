/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#define _GNU_SOURCE
#include "sch-helpers.h"
#include <features.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/*Simulated CPU Core*/
struct CPUCore{
    pthread_t *CPUThread;
    process_queue *readyQ;
    process_queue *DeviceQ;
    process_queue *completedQ;
    int totalCPUBurst;
    bool cpuRunning;
    int *t;
    bool *systemInitialized;
    bool *systemRunning;
};

struct IO{
    process_queue *readyQ;
    process_queue *DeviceQ;
    process_queue *completedQ;
    int totalIOBurst;
    bool IORunning;
    int *t;
    bool *systemInitialized;
    bool *systemRunning;
    
};

struct Clock{
    int *t;
    bool *running;
};

struct ArrivalStruct{
    process_queue *q;
    int numberOfProcesses;
    int *t;
    process *pArray;
};


/*Threaded Functions*/
void* TimerThread(void*arg);
void* ArrivalThread(void*arg);
void* CPU(void*arg);
void* IOsim(void*arg);

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

    printf("successful up till now 1");


    int time=0;
    bool running = true;
    struct Clock *timer = (struct Clock *) malloc (sizeof(struct Clock));
    struct ArrivalStruct *arrivals = (struct ArrivalStruct *) malloc (sizeof(struct ArrivalStruct));
    timer->t = arrivals->t = &time;
    timer->running = &running;
    arrivals->pArray = processes;
    arrivals->q=readyQ;
    arrivals->numberOfProcesses = numberOfProcesses;

    struct CPUCore * cpus = (struct CPUCore *) malloc(NUMBER_OF_PROCESSORS * sizeof(struct CPUCore));
    bool systemInitialized  = false;

    struct IO *devIO = (struct IO *) malloc(sizeof(struct IO));


    /*Generate CPU Cores*/
    for (int i = 0; i < NUMBER_OF_PROCESSORS; i++) {
        cpus[i].CPUThread = (pthread_t *)malloc(sizeof(pthread_t));
        cpus[i].readyQ = readyQ;
        cpus[i].DeviceQ = DeviceQ;
        cpus[i].completedQ = completedQ;
        cpus[i].totalCPUBurst = 0;
        cpus[i].cpuRunning = false;
        cpus[i].t = &time;
        cpus[i].systemInitialized = &systemInitialized;
        cpus[i].systemRunning = &running;
    }

    /*Intialize and create IO, timer and arrival threads*/
    pthread_t timerThread;
    pthread_t arrivalThread;
    pthread_t IOthread;


    printf("successful up till now 2");

    /*Create thread and check if it is creater properly*/
    if (pthread_create(&timerThread, NULL, TimerThread, (void *)timer) != 0) {
                fprintf(stderr, "Failed to create thread for timer\n");
                return 1;
    }
    if (pthread_create(&arrivalThread, NULL, ArrivalThread, (void *)arrivals) != 0) {
                fprintf(stderr, "Failed to create thread for arrivals\n");
                return 1;
    }
    if (pthread_create(&IOthread, NULL, IOsim, (void *)devIO) != 0) {
                fprintf(stderr, "Failed to create thread for IO\n");
                return 1;
    }



    /*intalize and create Simulated CPU Cores*/
    for (int i = 0; i < NUMBER_OF_PROCESSORS; i++) {
        pthread_create(cpus[i].CPUThread, NULL, CPU, (void *)&cpus[i]);
    }
    systemInitialized = true;
    while(completedQ->size <= numberOfProcesses){
        usleep(1000);//sleep for 1 ms
    }
    running=false;
    
    // Join threads back together
    pthread_join(timerThread, NULL);
    pthread_join(arrivalThread, NULL);

    for (int i = 0; i < NUMBER_OF_PROCESSORS; i++) {
        pthread_join(*(cpus[i].CPUThread), NULL);
        free(cpus[i].CPUThread);
    }


    printf("successful up till now 3");

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

    /*Free Allocated Memory*/
    free(timer);
    free(arrivals);
    free(readyQ);
    free(DeviceQ);
    free(completedQ);
    free(cpus);
    
return 0;

}


/************************************************END OF MAIN******************************************/



/*Simulated Clock Thread*/
void* TimerThread (void*arg){
    struct Clock* trueClock =(struct Clock *)arg;
    /*Generate simulated clock/timer*/
    while(*(trueClock->running)){
        usleep(1000); //Sleep for 1ms
        (*(trueClock->t))++;
    }
    return NULL;
}


/*Simulated Processor arrival thread*/
void* ArrivalThread(void*arg){
    struct ArrivalStruct* arrived = (struct ArrivalStruct *)arg;
    int processNum=0;
    /*Add processes to readyQ based on Arrival time*/
    while(processNum < arrived->numberOfProcesses){
        if(*(arrived->t)>= arrived->pArray[processNum].arrivalTime){
            enqueueProcess(arrived->q,&(arrived->pArray[processNum]));
            processNum++;
        }
        else{
            usleep(1000);//Sleep for 1ms
        }
    }
    return NULL;
}


/*Simulated CPU Thread*/
void* CPU(void*arg){
    struct CPUCore * core = (struct CPUCore *)arg;
    process *currentProcess;


    while(*(core->systemRunning)){
        if(*(core->systemInitialized)){
           
           if (core->readyQ->size > 0) {
                currentProcess = core->readyQ->front->data;
                dequeueProcess(core->readyQ);

                /*populate Process data*/
                if (currentProcess->startTime == 0){
                    currentProcess->startTime = *(core->t);
                    currentProcess->waitingTime = currentProcess->startTime - currentProcess->arrivalTime;
                } 
                else{
                    currentProcess->waitingTime += (*(core->t)-currentProcess->lastRequeTime);
                }

                /*Simulate CPU Burst*/
                core->cpuRunning = true;
                usleep(currentProcess->bursts[currentProcess->currentBurst].length * 1000);  // Simulate CPU burst
                core->cpuRunning = false;
                core->totalCPUBurst += currentProcess->bursts[currentProcess->currentBurst].length;

                /*check if there is further IO. Move to appropriate Queue.*/
                if(currentProcess->currentBurst==currentProcess->numberOfBursts){ //Process Complete
                    currentProcess->endTime = *(core->t);
                    enqueueProcess(core->completedQ,currentProcess);
                }
                else if((currentProcess->currentBurst < currentProcess->numberOfBursts)&&!(currentProcess->currentBurst % 2)){
                        enqueueProcess(core->DeviceQ,currentProcess);
                        currentProcess->currentBurst++;
                }
                else{
                    printf("Process error for pid%d!!\n",currentProcess->pid);
                }
           }
           else{
            usleep(1000);
           }
        }
        else {
            usleep(1000);
        }
    }
    return NULL;
}


/*IO Handler Thread. Assumes all 4 CPU Cores share IO*/
void* IOsim(void*arg){
    struct IO * io = (struct IO *)arg;
    process *currentProcess;


    while(*(io->systemRunning)){
        if(*(io->systemInitialized)){
            if (io->DeviceQ->size > 0) {
                currentProcess = io->DeviceQ->front->data;
                dequeueProcess(io->DeviceQ);

                /*Simulate IO Burst*/
                io->IORunning = true;
                usleep(currentProcess->bursts[currentProcess->currentBurst].length * 1000);  // Simulate IO burst
                io->IORunning = false;


                /*check if there is further CPU. Move to appropriate Queue.*/
                if(currentProcess->currentBurst==currentProcess->numberOfBursts){ //Process Complete
                    currentProcess->endTime = *(io->t);
                    enqueueProcess(io->completedQ,currentProcess);
                }
                else if((currentProcess->currentBurst < currentProcess->numberOfBursts)&&(currentProcess->currentBurst % 2)){
                        currentProcess->lastRequeTime = *(io->t);
                        enqueueProcess(io->readyQ,currentProcess);
                        currentProcess->currentBurst++;
                }
                else{
                    printf("Process error for pid%d!!\n",currentProcess->pid);
                }
            }
            else {
                usleep(1000);
            }

        }
        else {
            usleep(1000);
        }
    }
    return NULL;


}
