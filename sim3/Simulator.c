// header files
#include "Simulator.h"

////////////////////////////////////////////////////////////////////////////////
// main function
////////////////////////////////////////////////////////////////////////////////
void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr)
{
    // initialize variables
    bool logToMonitor = false, logToFile = false, memDisplay, memErr = false;
    PCBTypeLinkedList *pcbLinkedList;
    FileOutTypeLinkedList *fileOutputList;
    PCBType *wkgPCBPtr;

    pthread_t tid;
    pthread_attr_t attr;

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

    memDisplay = configPtr->memDisplay;

    // initialize all processes to new state and initialize fileOutList
    pcbLinkedList = setProcessesToNewState(configPtr, metaDataMstrPtr);
    fileOutputList = initializeFileLinkedList();
    wkgPCBPtr = pcbLinkedList->headPtr;

    if(!logToMonitor && logToFile)
    {
        printf("Simulator running to file output only\n");
        printf("Please wait...\n");
    }

    // start sim and start timer
    displaySimStartOrEnd(logToMonitor, fileOutputList, START);

    // set all processes to READY
    while(wkgPCBPtr != NULL)
    {
        wkgPCBPtr->state = READY_STATE;
        displayChangedState(logToMonitor, fileOutputList, wkgPCBPtr->state, 
                                                                wkgPCBPtr->pid);
        wkgPCBPtr = wkgPCBPtr->nextNode;
    }

    // initialize the memory
    displayMemOp(memDisplay, logToMonitor, configPtr, fileOutputList, INIT, 
                                                             memErr, wkgPCBPtr);

    // reset wkgPtr to beginning of linked list
    wkgPCBPtr = pcbLinkedList->headPtr;

    // master loop (while not all processes in EXIT state)
    while(!allProcessesInState(EXIT_STATE, pcbLinkedList))
    {
        // get next process if nothing is running //////////////////////////////
        if(wkgPCBPtr->state != RUNNING_STATE)
        {
            wkgPCBPtr = selectProcessToRun(configPtr, pcbLinkedList);
            displayProcessSelected(wkgPCBPtr, fileOutputList, logToMonitor);
            wkgPCBPtr->state = RUNNING_STATE;
            displayChangedState(logToMonitor, fileOutputList, RUNNING_STATE, 
                                                                wkgPCBPtr->pid);
            memErr = false;
        }

        // check for i/o ops or cpu ops/////////////////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->command, "dev") == STR_EQ
        || compareString(wkgPCBPtr->programCounter->command, "cpu") == STR_EQ)
        {
            displayOpcodeLine(logToMonitor, fileOutputList, wkgPCBPtr, START);

            // wait till process is done using threads
            pthread_attr_init(&attr);
            pthread_create(&tid, &attr, runOpCmdForTime, wkgPCBPtr);
            pthread_join(tid,NULL);

            displayOpcodeLine(logToMonitor, fileOutputList, wkgPCBPtr, END);
        }

        // check for memory ops ////////////////////////////////////////////////
        else if(compareString(wkgPCBPtr->programCounter->command, "mem") 
                                                                      == STR_EQ)
        {
            displayOpcodeLine(logToMonitor, fileOutputList, wkgPCBPtr, START);

            if(compareString(wkgPCBPtr->programCounter->strArg1, "allocate")
                                                                      == STR_EQ)
            {
                // allocate memory operation/display
                memErr = allocateMemory(wkgPCBPtr, configPtr);
                displayMemOp(memDisplay, logToMonitor, configPtr, 
                                   fileOutputList, ALLOCATE, memErr, wkgPCBPtr);
            }

            else
            {
                // access memory operation/display
                memErr = accessMemory(wkgPCBPtr);
                displayMemOp(memDisplay, logToMonitor, configPtr, 
                                     fileOutputList, ACCESS, memErr, wkgPCBPtr);
            }

            // decide what print message to output fail/success
            if(memErr)
            {
                displayOpcodeLine(logToMonitor, fileOutputList, wkgPCBPtr, 
                                                                          FAIL);
            }

            else
            {
                displayOpcodeLine(logToMonitor, fileOutputList, wkgPCBPtr, 
                                                                       SUCCESS);
            }
        }

        // check for completion or memory failed ///////////////////////////////
        if(compareString(wkgPCBPtr->programCounter->strArg1, "end") == STR_EQ ||
                                                                         memErr)
        {
            displayProcessEnd(logToMonitor, fileOutputList, wkgPCBPtr, memErr);

            displayMemOp(memDisplay, logToMonitor, configPtr, fileOutputList, 
                                                      CLEAR, memErr, wkgPCBPtr);

            wkgPCBPtr->state = EXIT_STATE;
            displayChangedState(logToMonitor, fileOutputList, EXIT_STATE, 
                                                                wkgPCBPtr->pid);
        }

        // increment to next command
        wkgPCBPtr->programCounter = wkgPCBPtr->programCounter->nextNode;
    }

    // stop timer and stop sim
    displaySystemStop(logToMonitor, fileOutputList);

    displayMemOp(memDisplay, logToMonitor, configPtr, fileOutputList, FREE, 
                                                             memErr, wkgPCBPtr);

    displaySimStartOrEnd(logToMonitor, fileOutputList, END);
    
    // display all output to a file if specified
    if(logToFile)
    {
        // output results to file
        displayOutputToFile(fileOutputList, configPtr->logToFileName);
    }

    // free all allocated data
    clearPCBLinkedList(pcbLinkedList);
    clearFileOutLinkedList(fileOutputList);

    // end simulation
}

////////////////////////////////////////////////////////////////////////////////
// function implementations
////////////////////////////////////////////////////////////////////////////////

bool allocateMemory(PCBType *wkgPCBPtr, ConfigDataType *configPtr)
{
    // initialize variables
    int logBase, logLimit, maxPhyAddress = 0;
    int currentLogBase, currentLogLim;
    int currentPhyBase, currentPhyLim;
    int index;

    // get values
    logBase = wkgPCBPtr->programCounter->intArg2;
    logLimit = wkgPCBPtr->programCounter->intArg3 - 1;

    // find if any other memory is allocated for that process
    for(index = 0; index < wkgPCBPtr->numSegments; index++)
    {
        currentLogBase = wkgPCBPtr->segments[index].logBase;
        currentLogLim = wkgPCBPtr->segments[index].logLim;
        currentPhyBase = wkgPCBPtr->segments[index].phyBase;
        currentPhyLim = wkgPCBPtr->segments[index].phyLim;

        if(maxPhyAddress < currentPhyBase + currentPhyLim)
        {
            maxPhyAddress = currentPhyBase + currentPhyLim;
        }

        // check if not overlapping logical address
        if((logBase >= currentLogBase && 
            logBase < currentLogBase + currentLogLim) ||
            (logBase + logLimit > currentLogBase && 
            logBase + logLimit <= currentLogBase + currentLogLim))
        {
            return true;
        }
    }

    // check if in range of max memory for physical address
    if(configPtr->memAvailable < maxPhyAddress + logLimit)
    {
        return true;
    }

    // add allocated memory to the process
    wkgPCBPtr->segments[wkgPCBPtr->numSegments].logBase = logBase;
    wkgPCBPtr->segments[wkgPCBPtr->numSegments].logLim = logLimit;
    wkgPCBPtr->segments[wkgPCBPtr->numSegments].phyBase = maxPhyAddress;
    wkgPCBPtr->segments[wkgPCBPtr->numSegments].phyLim = logLimit;
    wkgPCBPtr->numSegments++;

    // return false for no errors
    return false;
}


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

bool accessMemory(PCBType *wkgPCBPtr)
{
    //initialize memory
    int requestBase, requestLimit, requestEnd;
    int currentLogBase, currentLogLim;
    int index;

    // get values
    requestBase = wkgPCBPtr->programCounter->intArg2;
    requestLimit =  wkgPCBPtr->programCounter->intArg3 - 1;
    requestEnd = requestBase + requestLimit;

    // check if base and limit within bounds of all other logical addresses
    for(index = 0; index < wkgPCBPtr->numSegments; index++)
    {
        currentLogBase = wkgPCBPtr->segments[index].logBase;
        currentLogLim = wkgPCBPtr->segments[index].logLim;

        // check if request falls entirely within the current segment
        if (requestBase >= currentLogBase && 
                                   requestEnd <= currentLogBase + currentLogLim)
        {
            return false; // valid access: total overlap detected
        }
    }

    return true; // invalid access
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
            wkgPtr->intArg2 = addedTime;
        }

        // increment the wkgPtr and add value to totalTime
        totalTime = addedTime + totalTime;
        wkgPtr = wkgPtr->nextNode; 
    }

    return totalTime;
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

void displayChangedState(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                   ProcessState state, int pid)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // check to display READY case
    if(state == READY_STATE)
    {
        sprintf(outputStr,
         "%s, OS: Process %d set to READY state from NEW state\n", 
                                                               timeString, pid);
    }

    // check to display running case
    else if(state == RUNNING_STATE)
    {
        sprintf(outputStr, "%s, OS: Process %d set from READY to RUNNING\n\n",
                                                              timeString, pid );
    }

    // check to display EXIT case
    else if(state == EXIT_STATE)
    {
        sprintf(outputStr, "%s, OS: Process %d set to EXIT\n", timeString, pid);
    }

    // assume error
    else
    {
        sprintf(outputStr, "Error with function displayChangedState\n");
    }

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr); 
}

////////////////////////////////////////////////////////////////////////////////

bool displayMemOp(bool displayMem, bool display, ConfigDataType *configPtr,
                       FileOutTypeLinkedList *fileOutputList, memOpStates opCmd, 
                                                 bool error, PCBType *wkgPCBPtr)
{
    // initialize variables
    char outputStr[HUGE_STR_LEN];
    char tempStr[LARGE_STR_LEN];
    bool displayUsed = false, displayOpen = false;
    int openStart;

    // check if memory should be displayed
    if(!displayMem)
    {
        return false;
    }

    sprintf(outputStr, "--------------------------------------------------\n");

    // find what memory statement to output
    switch(opCmd)
    {
        case INIT:
            concatenateString(outputStr, "After memory initialization\n");
            openStart = 0;
            displayOpen = true;
            break;

        case FREE:
            concatenateString(outputStr, "After clear all process success\n");
            concatenateString(outputStr, "No memory configured\n");
            break;

        case CLEAR:
            sprintf(tempStr, "After clear process %d success\n", 
                                                                wkgPCBPtr->pid);
            concatenateString(outputStr, tempStr);
            openStart = 0;
            displayOpen = true;
            break;
        
        case ALLOCATE:
            if(!error)
            {
                concatenateString(outputStr, "After allocate success\n");
            }

            else
            {
                concatenateString(outputStr, 
                                            "After allocate overlap failure\n");
            }
            openStart = 
                     wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].phyLim + 1;
            displayOpen = true;
            displayUsed = true;
            break;

        case ACCESS:
            if(!error)
            {
                concatenateString(outputStr, "After access success\n");
            }

            else
            {
                concatenateString(outputStr, "After access failure\n");
            }
            openStart = 
                     wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].phyLim + 1;
            displayOpen = true;
            displayUsed = true;
            break;
    }

    // check to display used and open spots in memory
    if(displayUsed)
    {
        sprintf(tempStr, "%d [ Used, P#: %d, %d-%d ] %d\n",
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].phyBase,
                    wkgPCBPtr->pid,
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].logBase,
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].logBase +
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].logLim,
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].phyBase +
                    wkgPCBPtr->segments[wkgPCBPtr->numSegments - 1].phyLim);

        concatenateString(outputStr, tempStr);
    }
    
    if(displayOpen)
    {
        sprintf(tempStr, "%d [ Open, P#: x, 0-0 ] %d\n",
                                          openStart, configPtr->memAvailable-1);
        concatenateString(outputStr, tempStr);
    }

    concatenateString(outputStr,
                        "--------------------------------------------------\n");

    // check to display process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    insertFileOutputNode(fileOutputList, outputStr);

    return true;
}


////////////////////////////////////////////////////////////////////////////////

void displayOutputToFile(FileOutTypeLinkedList *fileOutputList, 
                                                           const char *fileName)
{
    // initialize variables
    FILE* file;
    FileOutType *wkgPtr;

        // initialize write only constant
        const char WRITE_ONLY_FLAG[] = "w";

    // open the file
    file = fopen(fileName, WRITE_ONLY_FLAG);

    // check for opening file error
    if(file == NULL)
    {
        printf("Error opening file\n");
    }

    // assume file is ready
    else
    {
        // get the headPtr;
        wkgPtr = fileOutputList->headPtr;

        // iterate through each line
        while(wkgPtr != NULL)
        {
            fprintf(file, "%s", wkgPtr->outLine);
            wkgPtr = wkgPtr->nextNode;
        }

        // close the file
        fclose(file);
    }
}


////////////////////////////////////////////////////////////////////////////////

void displayOpcodeLine(bool display, 
                            FileOutTypeLinkedList *fileOutputList, 
                                                PCBType *pcb, SimOpStates state)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[MAX_STR_LEN];
    char stateString[MIN_STR_LEN];

    // get time
    accessTimer(LAP_TIMER, timeString);

    // check what state it is in
    switch (state)
    {
        case START:
            copyString(stateString, "start");
            break;
        case END:
            copyString(stateString, "end");
            break;
        case SUCCESS:
            copyString(stateString, "successful");
            break;
        default:
            copyString(stateString, "failed");
            break;
    }

    // check if displaying dev
    if(compareString(pcb->programCounter->command, "dev") == STR_EQ)
    {
        sprintf(outputStr, "%s, Process: %d, %s %sput operation %s\n",
                                          timeString, pcb->programCounter->pid,
                                          pcb->programCounter->strArg1, 
                                          pcb->programCounter->inOutArg,
                                          stateString);
    }

    // check if displaying cpu
    else if(compareString(pcb->programCounter->command, "cpu") == STR_EQ)
    {
        sprintf(outputStr, "%s, Process: %d, cpu process operation %s\n",
                                    timeString, pcb->programCounter->pid, 
                                                                   stateString);
    }

    // assume displaying mem 
    else
    {
        if(state == START)
        {
            sprintf(outputStr, "%s, Process: %d, mem %s request (%d, %d)\n",
                                            timeString, pcb->pid, 
                                            pcb->programCounter->strArg1,
                                            pcb->programCounter->intArg2,
                                            pcb->programCounter->intArg3);
        }

        else
        {
            sprintf(outputStr, "%s, Process: %d, %s mem %s request\n",
                                            timeString, pcb->pid, stateString,
                                            pcb->programCounter->strArg1);
        }
    }

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr); 
}


////////////////////////////////////////////////////////////////////////////////

void displayProcessEnd(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                      PCBType *pcb, bool memErr)
{
    //initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    accessTimer(LAP_TIMER, timeString);

    if(!memErr)
    {
        sprintf(outputStr, "\n%s, OS: Process %d ended\n", 
                                                          timeString, pcb->pid);
    }
    
    else
    {
        sprintf(outputStr, "\n%s, OS: Segmentation fault, Process %d ended\n", 
                                                          timeString, pcb->pid);
    }

    // check if display to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr);
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

    // check if display to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr); 
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
    else if(state == END)
    {
        accessTimer(STOP_TIMER, timeString);
        sprintf(outputStr, "%s, OS: Simulation end\n", timeString);
    }

    // assume error
    else
    {
        sprintf(outputStr, "Error with function displaySimStartOrEnd\n");
    }

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr); 
}


////////////////////////////////////////////////////////////////////////////////

void displaySystemStop(bool display, FileOutTypeLinkedList *fileOutputList)
{
    // initialize variables
    char timeString[MIN_STR_LEN], outputStr[LARGE_STR_LEN];

    // set output string
    accessTimer(LAP_TIMER, timeString);
    sprintf(outputStr, "%s, OS: System stop\n", timeString);

    // check to display the process to monitor
    if(display)
    {
        printf("%s", outputStr);
    }

    // add string to fileOutputList
    insertFileOutputNode(fileOutputList, outputStr); 
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

bool insertFileOutputNode(FileOutTypeLinkedList *linkedList, const char *output)
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
    int index;

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
    newNode->numSegments = 0;

    for (index = 0; index < MAX_SEGMENTS; index++)
    {
        newNode->segments[index].phyBase = -1;
        newNode->segments[index].phyLim = -1;
        newNode->segments[index].logBase = -1;
        newNode->segments[index].logLim = -1;
    }

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

void *runOpCmdForTime(void *param)
{
    // initialize variables
    char timeString[MIN_STR_LEN];
    PCBType *wkgPCBPtr = (PCBType *)param;
    int timeValue = wkgPCBPtr->programCounter->intArg2;
    double startTime, endTime, currentTime;
    double timeDuration;

    const double THOUSANDTH_MULT = 0.001;

    // calculate the duration of opcmd
    timeDuration = timeValue * THOUSANDTH_MULT;

    // get the start time
    startTime = accessTimer(LAP_TIMER, timeString);
    currentTime = startTime;

    // calculate the end time
    endTime = startTime + timeDuration;

    // loop until time complete
    while(currentTime < endTime)
    {
        currentTime = accessTimer(LAP_TIMER, timeString);
    }
    
    wkgPCBPtr->programCounter->opEndTime = currentTime;

    // subtract from the current time here
    wkgPCBPtr->totalTime = 
                    (wkgPCBPtr->totalTime - wkgPCBPtr->programCounter->intArg2);

    // exit the thread
    pthread_exit(0);
}


////////////////////////////////////////////////////////////////////////////////

PCBType *selectProcessFCFSNSched(PCBTypeLinkedList *pcbLinkedList)
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

PCBType *selectProcessSJFNSched(PCBTypeLinkedList *pcbLinkedList)
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
                                               PCBTypeLinkedList *pcbLinkedList)
{
    //initialize variables
    PCBType *pcbSelected;

    // find what scheduling algorithm to use
    switch (configPtr->cpuSchedCode)
    {
        case CPU_SCHED_FCFS_N_CODE:
            
            // first come first served non-preemptive
            pcbSelected = selectProcessFCFSNSched(pcbLinkedList);
            break;

        case CPU_SCHED_SJF_N_CODE:

            // shortest job first non-preemptive
            pcbSelected = selectProcessSJFNSched(pcbLinkedList);
            break;

        default:

            // error, set to NULL
            pcbSelected = NULL;
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