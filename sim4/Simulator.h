// protect from multiple compiling
#ifndef SIMULATOR_H
#define SIMULATOR_H

// header files
#include "StandardConstants.h"
#include "Configops.h"
#include "Metadataops.h"
#include "Simtimer.h"
#include <pthread.h>

// constants
typedef enum {START, END, QUANTUM_TIME_LIMIT, INTERRUPT} SimOpStates;
typedef enum {OS, PROCESS, MEM, NEWLINE} DisplayCodes;
# define EMPTY -1
# define THOUSANDTH_MULT 0.001

////////////////////////////////////////////////////////////////////////////////
// data structures
////////////////////////////////////////////////////////////////////////////////

// linked list for all processes (required PCB structure)
typedef struct PCBType
{
    int pid;                            // process identification
    int totalTime;                      // process times added up for PCB

    OpCodeType *programCounter;         // tracks location in process
    ProcessState state;                 // tracks what state the process is in

    struct PCBType *nextNode;           // points to next PCBtype

} PCBType;

typedef struct PCBTypeLinkedList
{
    PCBType *headPtr;                   // saves location of the headPtr

} PCBTypeLinkedList; 

// queue struct for process selection
typedef struct Queue
{
    PCBType *front;                     // front of ready queue
    PCBType *rear;                      // end of ready queue

} Queue;

// linked list for all file output 
typedef struct FileOutType
{
    char outLine[HUGE_STR_LEN];          // string to be outputted
    DisplayCodes code;                   // display code for spacing

    struct FileOutType *nextNode;        // points to next string

} FileOutType;

typedef struct FileOutTypeLinkedList
{
    FileOutType *headPtr;                // saves location of the headPtr

} FileOutTypeLinkedList;

// thread parameter struct
typedef struct IOThread
{
    PCBType * pcb;                      // passes the PCB struct
    Queue *interruptQueue;              // passes the interrupt queue
    pthread_mutex_t *lock;              // Passes the mutex lock

} IOThread;

////////////////////////////////////////////////////////////////////////////////
// function prototypes
////////////////////////////////////////////////////////////////////////////////


bool allProcessesInState(ProcessState state, PCBTypeLinkedList *pcbLinkedList);


int calcTotalTime(ConfigDataType *configPtr, OpCodeType *metaDataPtr);


bool checkCPUIdle( PCBTypeLinkedList *pcbLinkedList);


DisplayCodes checkDisplayLine(FileOutTypeLinkedList *linkedList);


FileOutTypeLinkedList *clearFileOutLinkedList(FileOutTypeLinkedList 
                                                                   *linkedList);

PCBTypeLinkedList *clearPCBLinkedList(PCBTypeLinkedList *linkedList);


Queue *clearQueue(Queue *queue);


IOThread *clearThreadParms( IOThread *threadParms);


PCBType *copyPCBNode(const PCBType src);


void cpuWait(bool display, FileOutTypeLinkedList *fileOutputList, 
        Queue *interruptQueue, Queue *rdyQueue, ConfigDataType *configPtr,
                       pthread_mutex_t *lock, PCBTypeLinkedList *pcbLinkedList);


int dequeue(Queue *queue);


void displayCPUIdle(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                             SimOpStates state);


void displayChangedState(bool display, FileOutTypeLinkedList *fileOutputList, 
                       ProcessState prevState, ProcessState nextState, int pid);


void displayInterrupt(bool display, FileOutTypeLinkedList *fileOutputList,
                                                                  PCBType *pcb);

                                                        
void displayNewLineChar(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                             DisplayCodes code);


void displayOutputToFile(FileOutTypeLinkedList *fileOutputList, 
                                                     ConfigDataType *configPtr);


void displayProcessBlocked(bool display, FileOutTypeLinkedList *fileOutputList,
                                                                  PCBType *pcb);


void displayProcessLine(bool display, 
                            FileOutTypeLinkedList *fileOutputList, 
                                               PCBType *pcb, SimOpStates state);


void displayProcessEnd(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                                  PCBType *pcb);


void displayProcessSelected(PCBType *pcbSelected, 
                           FileOutTypeLinkedList *fileOutputList, bool display);


void displaySimStartOrEnd(bool display, 
                      FileOutTypeLinkedList *fileOutputList, SimOpStates state);


void displaySystemStop(bool display, FileOutTypeLinkedList *fileOutputList);


void displayQuantumTimeOut(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                                       int pid);


void enqueue(Queue *queue, PCBType* wkgPCBPtr);


PCBType *getPIDInformation(int pid, PCBTypeLinkedList *pcbLinkedList);


FileOutTypeLinkedList *initializeFileLinkedList();


PCBTypeLinkedList *initializePCBLinkedList();


Queue *initializeQueue();


IOThread * initializeThreadParms(PCBType * pcb, Queue * interruptQueue, 
                                                         pthread_mutex_t *lock);


bool insertFileOutputNode(FileOutTypeLinkedList *linkedList, 
                                         const char *output, DisplayCodes code);


bool insertPCBNode(PCBTypeLinkedList *linkedList, int pid, int totalTime, 
                                                       OpCodeType *metaDataPtr);


bool interruptHandler(bool display, FileOutTypeLinkedList *fileOutputList, 
                      Queue *interruptQueue, Queue *rdyQueue, 
                      bool wasIdle, pthread_mutex_t *lock, 
                      PCBTypeLinkedList *pcbLinkedList, 
                      ConfigDataType *configPtr);


bool isPreemptive(ConfigDataType *configPtr);


void runCpuCmdForTime(bool display, FileOutTypeLinkedList *fileOutputList, 
                             Queue *rdyQueue, PCBTypeLinkedList *pcbLinkedList, 
                             PCBType *wkgPCBPtr, ConfigDataType *configPtr, 
                             Queue *interruptQueue, pthread_mutex_t *lock, 
                             bool preemptive);


void *runIOCmdForTime(void *param);


void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr);


PCBType *selectProcessFCFSSched(PCBTypeLinkedList *pcbLinkedList);


PCBType *selectProcessRRPSched( PCBTypeLinkedList *pcbLinkedList, 
                                                               Queue *rdyQueue);


PCBType *selectProcessSJFSched(PCBTypeLinkedList *pcbLinkedList);


PCBType *selectProcessToRun(ConfigDataType *configPtr, 
                            PCBTypeLinkedList *pcbLinkedList, Queue * rdyQueue);


PCBTypeLinkedList *setProcessesToNewState(ConfigDataType *configPtr, 
                                                   OpCodeType *metaDataMstrPtr);

#endif // SIMULATOR_H