// header files
#include "Simulator.h"

////////////////////////////////////////////////////////////////////////////////
// main function
////////////////////////////////////////////////////////////////////////////////
void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr)
{
    // initialize variables
    bool logToMonitor = false, logToFile = false, preemptive, firstRun;
    PCBTypeLinkedList *pcbLinkedList;
    FileOutTypeLinkedList *fileOutputList;
    Queue * rdyQueue, *interruptQueue;
    PCBType *wkgPCBPtr;
    int prevPid;

    // initialize pthread variables
    pthread_t tid;
    pthread_attr_t attr;
    pthread_mutex_t interruptQueueLock;
    pthread_mutex_init(&interruptQueueLock, NULL);
    IOThread *ioParams;

    // display title 
    printf("Simulator Run\n");
    printf("-------------\n\n");

    // set configuration file output settings
    if(configPtr->logToCode == LOGTO_BOTH_CODE || 
                                    configPtr->logToCode == LOGTO_MONITOR_CODE)
    {
        logToMonitor = true;
    }

    if(configPtr->logToCode == LOGTO_BOTH_CODE || 
                                    configPtr->logToCode == LOGTO_FILE_CODE)
    {
        logToFile = true;
    }

    preemptive = isPreemptive(configPtr);

    // initialize all data structures
    pcbLinkedList = setProcessesToNewState(configPtr, metaDataMstrPtr);
    fileOutputList = initializeFileLinkedList();
    rdyQueue = initializeQueue();
    interruptQueue = initializeQueue();

    // if not displaying to monitor, display message
    if(!logToMonitor && logToFile)
    {
        printf("Simulator running to file output only\n");
        printf("Please wait...\n");
    }

    // start sim and start timer
    displaySimStartOrEnd(logToMonitor, fileOutputList, START);

    // set all processes to READY
    wkgPCBPtr = pcbLinkedList->headPtr;
    while(wkgPCBPtr != NULL)
    {
        wkgPCBPtr->state = READY_STATE;
        displayChangedState(logToMonitor, fileOutputList, NEW_STATE, 
                                                    READY_STATE,wkgPCBPtr->pid);
        enqueue(rdyQueue, wkgPCBPtr);
        wkgPCBPtr = wkgPCBPtr->nextNode;
    }

    // reset wkgPtr to beginning of linked list
    wkgPCBPtr = pcbLinkedList->headPtr;
    prevPid = EMPTY;
    firstRun = true;

    // master loop (while not all processes in EXIT state)
    while(!allProcessesInState(EXIT_STATE, pcbLinkedList))
    {
        // check for all processes blocked /////////////////////////////////////
        if(checkCPUIdle(pcbLinkedList))
        {
            // wait until an interrupt comes in
            displayCPUIdle(logToMonitor, fileOutputList, START);
            cpuWait(logToMonitor, fileOutputList, interruptQueue, rdyQueue, 
                                 configPtr, &interruptQueueLock, pcbLinkedList);
        }

        // get next ready process if nothing is running ////////////////////////
        if(wkgPCBPtr->state != RUNNING_STATE)
        {
            // select process to run
            prevPid = wkgPCBPtr->pid;
            wkgPCBPtr = selectProcessToRun(configPtr, pcbLinkedList, rdyQueue);
            wkgPCBPtr->state = RUNNING_STATE;

            // check if process selected is the same as before
            if(wkgPCBPtr->pid != prevPid || firstRun)
            {
                // display selected process if different
                displayProcessSelected(wkgPCBPtr, fileOutputList, logToMonitor);
                wkgPCBPtr->state = RUNNING_STATE;
                displayChangedState(logToMonitor, fileOutputList, READY_STATE, 
                                                 RUNNING_STATE, wkgPCBPtr->pid);
            }
        }

        // check for i/o ops ///////////////////////////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->command, "dev") == STR_EQ)
        {
            displayProcessLine(logToMonitor, fileOutputList, wkgPCBPtr, START);

            // change state to blocked
            displayProcessBlocked(logToMonitor, fileOutputList, wkgPCBPtr);
            wkgPCBPtr->state = BLOCKED_STATE;
            displayChangedState(logToMonitor, fileOutputList, RUNNING_STATE,
                                                 BLOCKED_STATE, wkgPCBPtr->pid);

            // wait till process is done using threads (explained below)

            // initialize a struct to pass through parameters
            ioParams = initializeThreadParms(wkgPCBPtr, interruptQueue, 
                                                           &interruptQueueLock);
            // define thread attributes
            pthread_attr_init(&attr);

            // create the thread passing parameters and function
            pthread_create(&tid, &attr, runIOCmdForTime, ioParams);

            // detach the thread to run independently
            pthread_detach(tid);

            // don't increment to next process
        }

        // check for cpu ops ///////////////////////////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->command, "cpu") == STR_EQ)
        {
            displayProcessLine(logToMonitor, fileOutputList, wkgPCBPtr, START);

            // run the process checking for completion, interrupts, and qt
            runCpuCmdForTime(logToMonitor, fileOutputList, rdyQueue, 
                             pcbLinkedList, wkgPCBPtr, configPtr, 
                             interruptQueue, &interruptQueueLock, preemptive);

            // will decide to increment or not in the run cpu cmd function
        }

        // check for mem ///////////////////////////////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->command, "mem") == STR_EQ)
        {
            // ignore and move to next command
            wkgPCBPtr->programCounter = wkgPCBPtr->programCounter->nextNode;
        }

        // check for start or end //////////////////////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->strArg1, "start") == STR_EQ)
        {
            // ignore start and move to next command
            wkgPCBPtr->programCounter = wkgPCBPtr->programCounter->nextNode;
        }

        if(compareString(wkgPCBPtr->programCounter->strArg1, "end") == STR_EQ)
        {
            displayProcessEnd(logToMonitor, fileOutputList, wkgPCBPtr);
            wkgPCBPtr->state = EXIT_STATE;
            displayChangedState(logToMonitor, fileOutputList, RUNNING_STATE, 
                                                    EXIT_STATE, wkgPCBPtr->pid);
        }

        // set first run to false
        firstRun = false;
    }

    // stop timer and stop sim
    displaySystemStop(logToMonitor, fileOutputList);
    displaySimStartOrEnd(logToMonitor, fileOutputList, END);
    
    // display all output to a file if specified
    if(logToFile)
    {
        // output results to file
        displayOutputToFile(fileOutputList, configPtr);
    }

    // free all allocated data
    clearPCBLinkedList(pcbLinkedList);
    clearFileOutLinkedList(fileOutputList);
    clearQueue(rdyQueue);
    clearQueue(interruptQueue);
    pthread_mutex_destroy(&interruptQueueLock);

    // end simulation
}

////////////////////////////////////////////////////////////////////////////////
// function implementations
////////////////////////////////////////////////////////////////////////////////

bool allProcessesInState(ProcessState state, PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    PCBType *wkgPtr;

    // set wkgPtr to headPtr
    wkgPtr = pcbLinkedList->headPtr;

    // iterate through linked list
    while(wkgPtr != NULL)
    {
        if(wkgPtr->state != state)
        {
            return false;
        }

        wkgPtr = wkgPtr->nextNode;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////

int calcTotalTime(ConfigDataType *configPtr, OpCodeType *metaDataPtr)
{
    // initialize variables
    OpCodeType *wkgPtr;
    int addedTime, totalTime = 0;

    // set wkgPtr to metaDataPtr
    wkgPtr = metaDataPtr;

    // iterate through all ops in PCB process
    while(wkgPtr != NULL && compareString(wkgPtr->strArg1, "end") != STR_EQ)
    {
        addedTime = 0;

        // check for I/O condition
        if(compareString(wkgPtr->inOutArg, "in") == STR_EQ ||
           compareString(wkgPtr->inOutArg, "out") == STR_EQ)
        {
            addedTime = wkgPtr->intArg2 * configPtr->ioCycleRate;
            wkgPtr->intArg2 = addedTime;
        }

        // check for process condition
        if(compareString(wkgPtr->strArg1, "process") == STR_EQ)
        {
            addedTime = wkgPtr->intArg2 * configPtr->procCycleRate;
        }

        // increment the wkgPtr and add value to totalTime
        totalTime = addedTime + totalTime;
        wkgPtr = wkgPtr->nextNode; 
    }

    return totalTime;
}

////////////////////////////////////////////////////////////////////////////////

bool checkCPUIdle( PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    PCBType *wkgPtr;

    // check if all processes are blocked or in exit state
    wkgPtr = pcbLinkedList->headPtr;
    while (wkgPtr != NULL)
    {
        // check if the CPU is not idle
        if(!(wkgPtr->state == BLOCKED_STATE || wkgPtr->state == EXIT_STATE))
        {
            return false;
        }

        wkgPtr = wkgPtr->nextNode;
    }

    // cpu is idle
    return true;    
}


////////////////////////////////////////////////////////////////////////////////

DisplayCodes checkDisplayLine(FileOutTypeLinkedList *linkedList)
{
    // initialize variables
    FileOutType *wkgPtr;

    // set wkgPtr to headPtr
    wkgPtr = linkedList->headPtr;

    // check if there are no lines
    if(wkgPtr == NULL)
    {
        return NEWLINE;
    }

    // assuming not empty go to the last line
    while(wkgPtr->nextNode != NULL)
    {
        // iterate to next wkgPtr
        wkgPtr = wkgPtr->nextNode;
    }

    // return code of last node
    return wkgPtr->code;
}


////////////////////////////////////////////////////////////////////////////////

FileOutTypeLinkedList *clearFileOutLinkedList(FileOutTypeLinkedList 
                                                                    *linkedList)
{
    // initialize variables
    FileOutType *wkgPtr, *tempPtr; 

    // set wkgPtr to headPtr
    wkgPtr = linkedList->headPtr;

    // deallocate memory for the FileOutType linked list
    while(wkgPtr != NULL)
    {
        // set the tempPtr to the nextNode;
        tempPtr = wkgPtr->nextNode;

        // free the wkgPtr
        free(wkgPtr);

        // set the wkgPtr as the tempPtr
        wkgPtr = tempPtr;
    }

    // deallocate memory for the FileOutTypeLinkedList structure
    free(linkedList);

    // return NULL to the calling function
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////

PCBTypeLinkedList *clearPCBLinkedList(PCBTypeLinkedList *linkedList)
{
    // initialize variables
    PCBType *wkgPtr, *tempPtr; 

    // set wkgPtr to headPtr
    wkgPtr = linkedList->headPtr;

    // deallocate memory for the PCBType linked list
    while(wkgPtr != NULL)
    {
        // set the tempPtr to the nextNode;
        tempPtr = wkgPtr->nextNode;

        // free the wkgPtr
        free(wkgPtr);

        // set the wkgPtr as the tempPtr
        wkgPtr = tempPtr;
    }

    // deallocate memory for the PCBTypeLinkedList structure
    free(linkedList);

    // return NULL to the calling function
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////

Queue *clearQueue(Queue *queue)
{
    // check if already cleared
    if (queue == NULL)
    {
        return NULL;
    }

    // while queue is not empty (redundant safe check)
    while(queue->front != NULL)
    {
        dequeue(queue);
    }

    // assume ready queue is empty
    free(queue);
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////

IOThread *clearThreadParms( IOThread *threadParms)
{
    // free back to memory and return NULL
    free(threadParms);
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////

PCBType *copyPCBNode(const PCBType src)
{
    // initialize variables
    PCBType *dest;

    // allocate memory for the copied node
    dest = (PCBType*)malloc(sizeof(PCBType));

    // copy data into the new copied node
    dest->nextNode = NULL;
    dest->pid = src.pid;
    dest->programCounter = src.programCounter;
    dest->state = src.state;
    dest->totalTime = src.totalTime;

    // return the copied PCB node
    return dest;
}

////////////////////////////////////////////////////////////////////////////////

void cpuWait(bool display, FileOutTypeLinkedList *fileOutputList, 
        Queue *interruptQueue, Queue *rdyQueue, ConfigDataType *configPtr,
                        pthread_mutex_t *lock, PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    bool foundInterrupt = false;
    bool wasIdle = true;

    // check for interrupt in queue
    while(!foundInterrupt)
    {
        
        foundInterrupt = interruptHandler(display, fileOutputList,
                                          interruptQueue, rdyQueue, wasIdle, 
                                          lock, pcbLinkedList, configPtr);
    }
}


////////////////////////////////////////////////////////////////////////////////

int dequeue(Queue *queue)
{
    // initialize variables
    PCBType * tempPtr;
    int pid;

    // check if the queue is empty
    if(queue->front == NULL)
    {
        return EMPTY;
    }

    // reassign nodes in queue
    tempPtr = queue->front;
    queue->front = queue->front->nextNode;

    // check if queue becomes empty
    if(queue->front == NULL)
    {
        queue->rear = NULL;
    }

    // free allocated (copied) ptr & return pid
    pid = tempPtr->pid;
    free(tempPtr);
    return pid;
}

////////////////////////////////////////////////////////////////////////////////

void displayCPUIdle(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                              SimOpStates state)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[MAX_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // check if start of idle
    if(state == START)
    {
        sprintf(outputStr, "%s, OS: CPU idle, all active processes blocked\n", 
                                                                    timeString);
    }

    // assume end of idle
    else
    {
        sprintf(outputStr, "%s, OS: CPU interrupt, end idle\n", timeString);
    }

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void displayChangedState(bool display, FileOutTypeLinkedList *fileOutputList, 
                        ProcessState prevState, ProcessState nextState, int pid)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // check to display new to ready case
    if(prevState == NEW_STATE && nextState == READY_STATE)
    {
        sprintf(outputStr,
         "%s, OS: Process %d set to READY state from NEW state\n", 
                                                               timeString, pid);
    }

    // check to display running to blocked case
    else if(prevState == RUNNING_STATE && nextState == BLOCKED_STATE)
    {
        sprintf(outputStr, "%s, OS: Process %d set from RUNNING to BLOCKED\n",
                                                              timeString, pid );
    }

    // check to display ready to running case
    else if(prevState == READY_STATE && nextState == RUNNING_STATE)
    {
        sprintf(outputStr, "%s, OS: Process %d set from READY to RUNNING\n",
                                                              timeString, pid );
    }

    // check to display blocked to ready case
    else if(prevState == BLOCKED_STATE && nextState == READY_STATE)
    {
        sprintf(outputStr, "%s, OS: Process %d set from BLOCKED to READY\n",
                                                              timeString, pid );
    }

    // assume display exit case
    else
    {
        sprintf(outputStr, "%s, OS: Process %d set to EXIT\n", timeString, pid);
    }

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void displayInterrupt(bool display, FileOutTypeLinkedList *fileOutputList,
                                                                  PCBType *pcb)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[MAX_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    sprintf(outputStr, 
                "%s, OS: Interrupted by process %d, %s %sput operation\n"
                           , timeString, pcb->pid, pcb->programCounter->strArg1, 
                                                 pcb->programCounter->inOutArg);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void displayProcessBlocked(bool display, FileOutTypeLinkedList *fileOutputList,
                                                                  PCBType *pcb)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[MAX_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // display blocked process
    sprintf(outputStr, "%s, OS: Process %d blocked for %sput operation\n",
                           timeString, pcb->pid, pcb->programCounter->inOutArg);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void displayNewLineChar(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                              DisplayCodes code)
{
    // initialize variables
    char outputStr[MIN_STR_LEN] = { NEWLINE_CHAR };
    DisplayCodes prevCode = checkDisplayLine(fileOutputList);

    // Check and display newline conditionally
    if (prevCode != NEWLINE && prevCode != code)
    {
        if (display)
        {
            printf("%s", outputStr);
        }
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, NEWLINE); 
}

////////////////////////////////////////////////////////////////////////////////

void displayOutputToFile(FileOutTypeLinkedList *fileOutputList, 
                                                      ConfigDataType *configPtr)
{
    // initialize variables
    FILE* file;
    FileOutType *wkgPtr;
    char cpuSchedCodeStr[MIN_STR_LEN];

        // initialize write only constant
        const char WRITE_ONLY_FLAG[] = "w";

    // open the file
    file = fopen(configPtr->logToFileName, WRITE_ONLY_FLAG);

    // check for opening file error
    if(file == NULL)
    {
        printf("Error opening file\n");
    }

    // assume file is ready
    else
    {
        // get cpu scheduling string
        switch(configPtr->cpuSchedCode)
        {
            case CPU_SCHED_SJF_N_CODE:
                copyString(cpuSchedCodeStr, "SJF-N");
                break;
            case CPU_SCHED_SRTF_P_CODE:
                copyString(cpuSchedCodeStr, "SRTF-P");
                break;
            case CPU_SCHED_FCFS_P_CODE:
                copyString(cpuSchedCodeStr, "FCFS-P");
                break;
            case CPU_SCHED_RR_P_CODE:
                copyString(cpuSchedCodeStr, "RR-P");
                break;
            case CPU_SCHED_FCFS_N_CODE:
                copyString(cpuSchedCodeStr, "FCFS-N");
                break;
        }

        // display header
        fprintf(file, "\n==================================================\n");
        fprintf(file, "Simulator Log File Header\n\n");
        fprintf(file, "File Name                       : %s\n", 
                                                   configPtr->metaDataFileName);
        fprintf(file, "CPU Scheduling                  : %s\n",
                                                               cpuSchedCodeStr);
        fprintf(file, "Quantum Cycles                  : %d\n", 
                                                      configPtr->quantumCycles);
        fprintf(file, "Memory Available (KB)           : %d\n", 
                                                       configPtr->memAvailable);
        fprintf(file, "Processor Cycle Rate (ms/cycle) : %d\n", 
                                                      configPtr->procCycleRate);
        fprintf(file, "I/O Cycle Rate (ms/cycle)       : %d\n\n", 
                                                        configPtr->ioCycleRate);
        fprintf(file, "================\nBegin Simulation\n\n");

        // get the headPtr;
        wkgPtr = fileOutputList->headPtr;

        // iterate through each line
        while(wkgPtr != NULL)
        {
            fprintf(file, "%s", wkgPtr->outLine);
            wkgPtr = wkgPtr->nextNode;
        }

        // display simulation end
        fprintf(file, "\nEnd Simulation - Complete\n");
        fprintf(file, "=========================\n");

        // close the file
        fclose(file);
    }
}


////////////////////////////////////////////////////////////////////////////////

void displayProcessLine(bool display, 
                            FileOutTypeLinkedList *fileOutputList, 
                                                PCBType *pcb, SimOpStates state)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[MAX_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // check if displaying start dev
    if(compareString(pcb->programCounter->command, "dev") == STR_EQ && 
                                                                 state == START)
    {
        sprintf(outputStr, "%s, Process: %d, %s %sput operation start\n",
                                          timeString, pcb->programCounter->pid,
                                          pcb->programCounter->strArg1, 
                                          pcb->programCounter->inOutArg);
    }

    // check if displaying start cpu
    else if(compareString(pcb->programCounter->command, "cpu") == STR_EQ &&
                                                                 state == START)
    {
        sprintf(outputStr, "%s, Process: %d, cpu process operation start\n",
                                    timeString, pcb->programCounter->pid);
    }

    // check if displaying end cpu
    else if(compareString(pcb->programCounter->command, "cpu") == STR_EQ &&
                                                                   state == END)
    {
        sprintf(outputStr, "%s, Process: %d, cpu process operation end\n",
                                    timeString, pcb->programCounter->pid);
    }

    // check to display newline char
    displayNewLineChar(display, fileOutputList, PROCESS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, PROCESS); 
}


////////////////////////////////////////////////////////////////////////////////

void displayProcessEnd(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                                   PCBType *pcb)
{
    //initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    accessTimer(LAP_TIMER, timeString);

    sprintf(outputStr, "%s, OS: Process %d ended\n", timeString, pcb->pid);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check if display to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS);
}


////////////////////////////////////////////////////////////////////////////////

void displayProcessSelected(PCBType *pcbSelected, 
                            FileOutTypeLinkedList *fileOutputList, bool display)
{
    //initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // construct string to display selected
    accessTimer(LAP_TIMER, timeString);
    sprintf(outputStr, "%s, OS: Process %d selected with %d ms remaining\n",
                          timeString, pcbSelected->pid, pcbSelected->totalTime);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check if display to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void displaySimStartOrEnd(bool display, 
                       FileOutTypeLinkedList *fileOutputList, SimOpStates state)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // check if start sim
    if(state == START)
    {
        accessTimer(ZERO_TIMER, timeString);
        sprintf(outputStr, "%s, OS: Simulator start\n", timeString);
    }

    // check if end sim
    else
    {
        accessTimer(STOP_TIMER, timeString);
        sprintf(outputStr, "%s, OS: Simulation end\n", timeString);
    }

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS);
}


////////////////////////////////////////////////////////////////////////////////

void displaySystemStop(bool display, FileOutTypeLinkedList *fileOutputList)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];
    
    // set output string
    accessTimer(LAP_TIMER, timeString);
    sprintf(outputStr, "%s, OS: System stop\n", timeString);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS);
}


////////////////////////////////////////////////////////////////////////////////

void displayQuantumTimeOut(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                                        int pid)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // set output string
    accessTimer(LAP_TIMER, timeString);

    // set up string
    sprintf(outputStr, 
        "%s, OS: Process %d quantum time out, cpu process operation end\n",
                                                               timeString, pid);

    // check to display newline char
    displayNewLineChar(display, fileOutputList, OS);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr, OS); 
}


////////////////////////////////////////////////////////////////////////////////

void enqueue(Queue *queue, PCBType* wkgPCBPtr)
{
    // initialize variables
    PCBType * PCBQueuePtr;

    // make a copy of the wkgPCBPtr
    PCBQueuePtr = copyPCBNode(*wkgPCBPtr);

    // check if the queue is not empty
    if(queue->front != NULL)
    {
        queue->rear->nextNode = PCBQueuePtr;
    }

    else
    {
        queue->front = PCBQueuePtr;
    }

    queue->rear = PCBQueuePtr;
}


////////////////////////////////////////////////////////////////////////////////

PCBType *getPIDInformation(int pid, PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    PCBType *wkgPtr;

    // iterate through all the processes
    wkgPtr = pcbLinkedList->headPtr;
    while(wkgPtr != NULL)
    {
        // check if the proccess has the same pid
        if(wkgPtr->pid == pid)
        {
            return wkgPtr;
        }

        wkgPtr = wkgPtr->nextNode;
    }

    return NULL;
}


////////////////////////////////////////////////////////////////////////////////

FileOutTypeLinkedList *initializeFileLinkedList()
{
    // initialize variables
    FileOutTypeLinkedList *newLinkedList;

    // allocate memory for the new linked list
    newLinkedList = (FileOutTypeLinkedList*)malloc(
                                                 sizeof(FileOutTypeLinkedList));
                                    
    // set the headPtr to NULL
    newLinkedList->headPtr = NULL;

    // return the newLinkedList
    return newLinkedList;
}


////////////////////////////////////////////////////////////////////////////////

PCBTypeLinkedList *initializePCBLinkedList()
{
    // initialize variables
    PCBTypeLinkedList *newLinkedList;

    // allocate memory for the new linked list
    newLinkedList = (PCBTypeLinkedList*)malloc(sizeof(PCBTypeLinkedList));
                                    
    // set the headPtr to NULL
    newLinkedList->headPtr = NULL;

    // return the newLinkedList
    return newLinkedList;
}


////////////////////////////////////////////////////////////////////////////////

Queue *initializeQueue()
{
    // initialize variables
    Queue *rq = (Queue*)malloc(sizeof(Queue));

    // set front and rear to NULL
    rq->front = NULL;
    rq->rear = NULL;
    return rq;
}


////////////////////////////////////////////////////////////////////////////////

IOThread * initializeThreadParms(PCBType * pcb, Queue * interruptQueue, 
                                                          pthread_mutex_t *lock)
{
    // initialize variables
    IOThread *threadParms = (IOThread*)malloc(sizeof(IOThread));

    // set up the struct
    threadParms->interruptQueue = interruptQueue;
    threadParms->pcb = pcb;
    threadParms->lock = lock;
    return threadParms;
}


////////////////////////////////////////////////////////////////////////////////

bool insertFileOutputNode(FileOutTypeLinkedList *linkedList, 
                                          const char *output, DisplayCodes code)
{
    // initialize variables
    FileOutType *wkgPtr, *newNode;

    // set wkgPtr to the headPtr
    wkgPtr = linkedList->headPtr;

    // create a new node
    newNode = (FileOutType*)malloc(sizeof(FileOutType));

    if(newNode == NULL)
    {
        return false;
    }

    // set data for new node
    copyString(newNode->outLine, output);
    newNode->nextNode = NULL;
    newNode->code = code;

    // check if list is empty, set new node as the head of the list
    if(linkedList->headPtr == NULL)
    {
        linkedList->headPtr = newNode;
    }

    // else, assume list isn't empty
    else
    {
        while(wkgPtr->nextNode != NULL)
        {
            wkgPtr = wkgPtr->nextNode;
        }

        wkgPtr->nextNode = newNode;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////

bool insertPCBNode(PCBTypeLinkedList *linkedList, int pid, int totalTime, 
                                                        OpCodeType *metaDataPtr)
{
    // initialize variables
    PCBType *wkgPtr, *newNode;

    // set wkgPtr to headPtr
    wkgPtr = linkedList->headPtr;

    // create a new node
    newNode = (PCBType*)malloc(sizeof(PCBType));

    if(newNode == NULL)
    {
        return false;
    }

    // set data for new node
    newNode->pid = pid;
    newNode->state = NEW_STATE;
    newNode->totalTime = totalTime;
    newNode->programCounter = metaDataPtr;
    newNode->nextNode = NULL;

    // check if list is empty, set new node as the head of the list
    if(linkedList->headPtr == NULL)
    {
        linkedList->headPtr = newNode;
    }

    // else, assume list isn't empty
    else
    {
        while(wkgPtr->nextNode != NULL)
        {
            wkgPtr = wkgPtr->nextNode;
        }

        wkgPtr->nextNode = newNode;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////

bool interruptHandler(bool display, FileOutTypeLinkedList *fileOutputList, 
                      Queue *interruptQueue, Queue *rdyQueue, 
                      bool wasIdle, pthread_mutex_t *lock, 
                      PCBTypeLinkedList *pcbLinkedList, 
                      ConfigDataType *configPtr)
{
    // initialize variables
    int pid;
    PCBType *pcb;
    bool interrupt = false;

    // lock for read write problem
    pthread_mutex_lock(lock);

    // dequeue and set process to ready for each process interrupted
    while(interruptQueue->front != NULL)
    {   
        // check if breaking cpu idle
        if(wasIdle && !interrupt)
        {
            displayCPUIdle(display, fileOutputList, END);
        }

        // dequeue the process
        pid = dequeue(interruptQueue);
        interrupt = true;
        pcb = getPIDInformation(pid, pcbLinkedList);

        // decrement from total time
        pcb->totalTime = pcb->totalTime - pcb->programCounter->intArg2;

        // display interrupt
        displayInterrupt(display, fileOutputList, pcb);

        // update process for completion of IO op
        pcb->programCounter = pcb->programCounter->nextNode;
        pcb->state = READY_STATE;
        
        // enqueue for possible preemptive process
        enqueue(rdyQueue, pcb);

        // display the changed state
        displayChangedState(display, fileOutputList, BLOCKED_STATE, 
                                                         READY_STATE, pcb->pid);
    }

    // unlock for read write problem
    pthread_mutex_unlock(lock);

    // return true if interrupt, false if not
    return interrupt;
}


////////////////////////////////////////////////////////////////////////////////

bool isPreemptive(ConfigDataType *configPtr)
{
    // check if non preemptive
    return (!(configPtr->cpuSchedCode == CPU_SCHED_FCFS_N_CODE ||
                              configPtr->cpuSchedCode == CPU_SCHED_SJF_N_CODE));
}


////////////////////////////////////////////////////////////////////////////////

void runCpuCmdForTime(bool display, FileOutTypeLinkedList *fileOutputList, 
                             Queue *rdyQueue, PCBTypeLinkedList *pcbLinkedList, 
                             PCBType *wkgPCBPtr, ConfigDataType *configPtr, 
                             Queue *interruptQueue, pthread_mutex_t *lock, 
                             bool preemptive)
{
    // initialize variables
    char timeString[MIN_STR_LEN];
    int cycleNum = 0;
    double endTime, currentTime, timeDuration;
    bool interrupt = false;

    // loop until an interrupt, quantum time (preemptive), or completion
    while (!interrupt && (wkgPCBPtr->programCounter->intArg2 > 0) &&
                           (!preemptive || cycleNum < configPtr->quantumCycles))
    {
        // run one process cycle
        timeDuration = configPtr->procCycleRate * THOUSANDTH_MULT;
        currentTime = accessTimer(LAP_TIMER, timeString);
        endTime = currentTime + timeDuration;

        while(currentTime < endTime)
        {
            currentTime = accessTimer(LAP_TIMER, timeString);
        }

        // decrement the cyclesLeft
        wkgPCBPtr->programCounter->intArg2 -= 1;

        // increment the cycle number
        cycleNum = cycleNum + 1;

        // decrement the total time
        wkgPCBPtr->totalTime -= configPtr->procCycleRate;

        // check for interrupts
        pthread_mutex_lock(lock);
        if(interruptQueue->front != NULL)
        {
            interrupt = true;
        }
        pthread_mutex_unlock(lock);
    }


    // check if hit interrupt
    if(interrupt)
    {
        // set process to ready and to end
        wkgPCBPtr->state = READY_STATE;
        displayProcessLine(display, fileOutputList, wkgPCBPtr, END);
        enqueue(rdyQueue, wkgPCBPtr);

        // handle necessary interrupts
        interruptHandler(display, fileOutputList, interruptQueue, rdyQueue, 
                                         false, lock, pcbLinkedList, configPtr);
    }

    // check if hit quantum time
    else if(preemptive && cycleNum >= configPtr->quantumCycles)
    {
        // set process to ready
        wkgPCBPtr->state = READY_STATE;
        enqueue(rdyQueue, wkgPCBPtr);
        displayQuantumTimeOut(display, fileOutputList, wkgPCBPtr->pid);
    }

    // assume opcmd finished
    else
    {
        // increment to next opcmd if process is done
        displayProcessLine(display, fileOutputList, wkgPCBPtr, END);
        wkgPCBPtr->programCounter = wkgPCBPtr->programCounter->nextNode;
    }
}


////////////////////////////////////////////////////////////////////////////////

void *runIOCmdForTime(void *param)
{
    // initialize variables
    char timeString[MIN_STR_LEN];
    IOThread *threadParms = (IOThread *)param;
    PCBType *pcb = threadParms->pcb;
    int timeValue = pcb->programCounter->intArg2;
    double endTime, currentTime, timeDuration;

    // calculate the duration of opcmd
    timeDuration = timeValue * THOUSANDTH_MULT;

    // get the start time
    currentTime = accessTimer(LAP_TIMER, timeString);

    // calculate the end time
    endTime = currentTime + timeDuration;

    // loop until time complete
    while(currentTime < endTime)
    {
        currentTime = accessTimer(LAP_TIMER, timeString);
    }

    // enqueue completed io cmd (protected)
    pthread_mutex_lock(threadParms->lock);
    enqueue(threadParms->interruptQueue, pcb);
    pthread_mutex_unlock(threadParms->lock);

    // clear allocated resources
    clearThreadParms(threadParms);

    // exit the thread
    pthread_exit(0);
}


////////////////////////////////////////////////////////////////////////////////

PCBType *selectProcessFCFSSched(PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    PCBType *wkgPtr, *lowestPCB;

    // set wkgPtr to headPtr
    wkgPtr = pcbLinkedList->headPtr;
    lowestPCB = NULL;

    // iterate through all processes in ready state
    while(wkgPtr != NULL)
    {
        // check if in READY state
        if(wkgPtr->state == READY_STATE)
        {
            // find what process arrived first (via pid)
            if(lowestPCB == NULL || wkgPtr->pid < lowestPCB->pid)
            {
                lowestPCB = wkgPtr;
            }
        }

        wkgPtr = wkgPtr->nextNode;
    }

    return lowestPCB;
}


////////////////////////////////////////////////////////////////////////////////

PCBType *selectProcessRRPSched( PCBTypeLinkedList *pcbLinkedList, 
                                                                Queue *rdyQueue)
{
    // initialize variables
    int pid;
    PCBType *dequeuedPCB;

    // get the next item out of the ready queue
    pid = dequeue(rdyQueue);
    dequeuedPCB = getPIDInformation(pid, pcbLinkedList);

    // return the dequeued process
    return dequeuedPCB;
}


////////////////////////////////////////////////////////////////////////////////

PCBType *selectProcessSJFSched(PCBTypeLinkedList *pcbLinkedList)
{
    // initialize variables
    PCBType *wkgPtr, *shortestJob;

    // set wkgPt to headPtr
    wkgPtr = pcbLinkedList->headPtr;
    shortestJob = NULL;

    // iterate through all processes in ready state
    while(wkgPtr != NULL)
    {
        // check if in the ready state
        if(wkgPtr->state == READY_STATE)
        {
            // check if processes compared have the same total time
            if(shortestJob != NULL && wkgPtr->totalTime == 
                                                         shortestJob->totalTime)
            {
                if(wkgPtr->pid < shortestJob->pid)
                {
                    shortestJob = wkgPtr;
                }
            }

            // find which process has the shorter total time
            else if(shortestJob == NULL || wkgPtr->totalTime < 
                                                         shortestJob->totalTime)
            {
                shortestJob = wkgPtr;
            }
        }

        wkgPtr = wkgPtr->nextNode;
    }

    return shortestJob;
}


////////////////////////////////////////////////////////////////////////////////

PCBType *selectProcessToRun(ConfigDataType *configPtr, 
                             PCBTypeLinkedList *pcbLinkedList, Queue * rdyQueue)
{
    //initialize variables
    PCBType *pcbSelected;

    // find what scheduling algorithm to use
    switch (configPtr->cpuSchedCode)
    {
        case CPU_SCHED_FCFS_N_CODE:
            
            // first come first served non-preemptive
            pcbSelected = selectProcessFCFSSched(pcbLinkedList);
            break;

        case CPU_SCHED_SJF_N_CODE:

            // shortest job first non-preemptive
            pcbSelected = selectProcessSJFSched(pcbLinkedList);
            break;

        case CPU_SCHED_FCFS_P_CODE:

            // first come first served preemptive (reuse FCFS algo)
            pcbSelected = selectProcessFCFSSched(pcbLinkedList);
            break;

        case CPU_SCHED_SRTF_P_CODE:

            // shortest remaining time first preemptive (reuse SJF algo)
            pcbSelected = selectProcessSJFSched(pcbLinkedList);
            break;

        case CPU_SCHED_RR_P_CODE:

            // add function here (need ready queue)
            pcbSelected = selectProcessRRPSched(pcbLinkedList, rdyQueue);
            break;
    }

    return pcbSelected;
}


////////////////////////////////////////////////////////////////////////////////

PCBTypeLinkedList *setProcessesToNewState(ConfigDataType *configPtr, 
                                                    OpCodeType *metaDataMstrPtr)
{
    // initialize variables
    PCBTypeLinkedList *pcbLinkedList;
    OpCodeType *wkgMetaDataPtr;
    int pid = 0, totalTime;

    // set workingPtr to master pointer
    wkgMetaDataPtr = metaDataMstrPtr;

    // initialize the linked list
    pcbLinkedList = initializePCBLinkedList();

    // iterate through all the metadata
    while(wkgMetaDataPtr != NULL)
    {
        // check if at the start of the process
        if(compareString(wkgMetaDataPtr->command, "app") == STR_EQ &&
           compareString(wkgMetaDataPtr->strArg1, "start") == STR_EQ)
        {
            // calculate total time
            totalTime = calcTotalTime(configPtr, wkgMetaDataPtr);

            // add PCB node
            insertPCBNode(pcbLinkedList, pid, totalTime, wkgMetaDataPtr);

            // go to the next process id to assigned
            pid = pid + 1;
        }

        // add the pid to the opcode type as well
        if(compareString(wkgMetaDataPtr->command, "sys") != STR_EQ)
        {
            wkgMetaDataPtr->pid = pid - 1;
        }

        // don't add pid to sys commands
        if(compareString(wkgMetaDataPtr->command, "sys") == STR_EQ)
        {
            wkgMetaDataPtr->pid = -1;
        }

        // iterate to the next node
        wkgMetaDataPtr = wkgMetaDataPtr->nextNode;
    }

    return pcbLinkedList;
}


////////////////////////////////////////////////////////////////////////////////