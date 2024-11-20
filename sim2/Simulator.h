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
typedef enum {START, END} SimOpStates;

////////////////////////////////////////////////////////////////////////////////
// data structures
////////////////////////////////////////////////////////////////////////////////

// linked list for all processes (required PCB structure)
typedef struct PCBType
{
    int pid;                            // process identification
    int totalTime;                      // process times added up for PCB

    OpCodeType *programCounter;         // tracks location in process
    ProcessState state;                // tracks what state the process is in

    struct PCBType *nextNode;           // points to next PCBtype

}PCBType;

typedef struct PCBTypeLinkedList
{
    PCBType *headPtr;                   // saves location of the headPtr

} PCBTypeLinkedList; 


// linked list for all file output /////////////////////////////////////////////
typedef struct FileOutType
{
    char outLine[MAX_STR_LEN];          // string to be outputted

    struct FileOutType *nextNode;       // points to next string

}FileOutType;

typedef struct FileOutTypeLinkedList
{
    FileOutType *headPtr;               // saves location of the headPtr

} FileOutTypeLinkedList; 

////////////////////////////////////////////////////////////////////////////////
// function prototypes
////////////////////////////////////////////////////////////////////////////////

/*
Name: allProcessesInState
Process: checks if all processes in the linked list are in the specified state
Function Input/Parameters: ProcessState state, PCBTypeLinkedList *pcbLinkedList
Function Output/Parameters: none
Function Output/Returned: bool returns true if all processes are in the 
                          specified state, otherwise returns false
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: none
*/
bool allProcessesInState(ProcessState state, PCBTypeLinkedList *pcbLinkedList);

/*
Name: calcTotalTime
Process: calculates the total time required for all operations in a PCB process
         based on configuration cycle rates
Function Input/Parameters: ConfigDataType *configPtr, OpCodeType *metaDataPtr
Function Output/Parameters: none
Function Output/Returned: total calculated time for the process operations 
                          (int)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: compareString
*/
int calcTotalTime(ConfigDataType *configPtr, OpCodeType *metaDataPtr);

/*
Name: clearFileOutLinkedList
Process: deallocates memory for the FileOutType linked list and its associated 
         FileOutTypeLinkedList structure
Function Input/Parameters: FileOutTypeLinkedList *linkedList 
Function Output/Parameters: none
Function Output/Returned: NULL pointer (FileOutTypeLinkedList *)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: free
*/
FileOutTypeLinkedList *clearFileOutLinkedList(FileOutTypeLinkedList 
                                                                   *linkedList);

/*
Name: clearPCBLinkedList
Process: deallocates memory for the PCBType linked list and its associated 
         PCBTypeLinkedList structure
Function Input/Parameters: PCBTypeLinkedList *linkedList 
Function Output/Parameters: none
Function Output/Returned: NULL pointer (PCBTypeLinkedList *)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: none
*/
PCBTypeLinkedList *clearPCBLinkedList(PCBTypeLinkedList *linkedList);

/*
Name: displayChangedState
Process: displays or logs a process state change (READY, RUNNING, EXIT) 
         for a process with the given PID
Function Input/Parameters: display (bool), FileOutTypeLinkedList *fileOutputList
                           ProcessState state, int pid
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays state change if the display flag is set to true
Dependencies: accessTimer, insertFileOutputNode, printf, sprintf
*/
void displayChangedState(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                   ProcessState state, int pid);

/*
Name: displayOutputToFile
Process: writes the contents of a FileOutType linked list to a specified file
Function Input/Parameters: fileOutputList (FileOutTypeLinkedList *), 
                           fileName (const char *)
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays error message if file cannot be opened
Dependencies: fopen, fprintf, fclose, printf
*/
void displayOutputToFile(FileOutTypeLinkedList *fileOutputList, 
                                                          const char *fileName);

/*
Name: displayOpcodeLine
Process: logs the start or end of a process operation (device or CPU) with 
         the associated process ID and time
Function Input/Parameters: display (bool), FileOutTypeLinkedList 
                           *fileOutputList, PCBType *pcb, SimOpStates state
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays operation log if the display flag is set to true
Dependencies: accessTimer, copyString, compareString, insertFileOutputNode, 
              printf, sprintf
*/
void displayOpcodeLine(bool display, 
                            FileOutTypeLinkedList *fileOutputList, 
                                               PCBType *pcb, SimOpStates state);

/*
Name: displayProcessEnd
Process: logs the end of a process with the associated process ID
Function Input/Parameters: display (bool), FileOutTypeLinkedList 
                           *fileOutputList, PCBType *pcb
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays process end log if the display flag is set to 
                       true
Dependencies: accessTimer, insertFileOutputNode, printf, sprintf
*/
void displayProcessEnd(bool display, FileOutTypeLinkedList *fileOutputList, 
                                                                  PCBType *pcb);

/*
Name: displayProcessSelected
Process: logs the selection of a process with its associated process ID 
         and remaining time
Function Input/Parameters: PCBType *pcbSelected, 
                           FileOutTypeLinkedList *fileOutputList,
                           bool display
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays process selection log if the display flag 
                       is set to true
Dependencies: accessTimer, insertFileOutputNode, printf, sprintf
*/
void displayProcessSelected(PCBType *pcbSelected, 
                           FileOutTypeLinkedList *fileOutputList, bool display);

/*
Name: displaySimStartOrEnd
Process: logs the start or end of the simulation based on the given state
Function Input/Parameters: bool display, FileOutTypeLinkedList *fileOutputList, 
                           SimOpStates state
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays simulation start or end log if the display 
                       flag is set to true
Dependencies: accessTimer, insertFileOutputNode, printf, sprintf
*/
void displaySimStartOrEnd(bool display, 
                      FileOutTypeLinkedList *fileOutputList, SimOpStates state);

/*
Name: displaySystemStop
Process: logs the system stop event
Function Input/Parameters: bool display, FileOutTypeLinkedList *fileOutputList
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: displays system stop log if the display flag is set 
                       to true
Dependencies: accessTimer, insertFileOutputNode, printf, sprintf
*/
void displaySystemStop(bool display, FileOutTypeLinkedList *fileOutputList);

/*
Name: initializeFileLinkedList
Process: creates and initializes a new FileOutTypeLinkedList
Function Input/Parameters: none
Function Output/Parameters: none
Function Output/Returned: pointer to the newly initialized 
                          FileOutTypeLinkedList
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: malloc
*/
FileOutTypeLinkedList *initializeFileLinkedList();

/*
Name: initializePCBLinkedList
Process: creates and initializes a new PCBTypeLinkedList
Function Input/Parameters: none
Function Output/Parameters: none
Function Output/Returned: pointer to the newly initialized 
                          PCBTypeLinkedList
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: malloc
*/
PCBTypeLinkedList *initializePCBLinkedList();

/*
Name: insertFileOutputNode
Process: inserts a new output node into a FileOutTypeLinkedList
Function Input/Parameters: FileOutTypeLinkedList *linkedList, 
                           const char *output
Function Output/Parameters: none
Function Output/Returned: true if the node is successfully inserted, 
                          false if memory allocation fails
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: malloc, copyString
*/
bool insertFileOutputNode(FileOutTypeLinkedList *linkedList, 
                                                            const char *output);

/*
Name: insertPCBNode
Process: inserts a new PCBType node into a PCBTypeLinkedList
Function Input/Parameters: PCBTypeLinkedList *linkedList, int pid, 
                           int totalTime, OpCodeType *metaDataPtr
Function Output/Parameters: none
Function Output/Returned: true if the node is successfully inserted, 
                          false if memory allocation fails
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: malloc
*/
bool insertPCBNode(PCBTypeLinkedList *linkedList, int pid, int totalTime, 
                                                       OpCodeType *metaDataPtr);

/*
Name: runOpCmdForTime
Process: simulates the execution of an operation command for a specified time
Function Input/Parameters: void *param (pointer to PCBType)
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: accessTimer, pthread_exit
*/
void *runOpCmdForTime(void *param);

/*
Name: runSim
Process: simulates the running of processes based on the given configuration 
         and metadata
Function Input/Parameters: ConfigDataType *configPtr
                           OpCodeType *metaDataMstrPtr
Function Output/Parameters: none
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: possible output depending on log settings 
                       (start/end of simulation, process states)
Dependencies: setProcessesToNewState, initializeFileLinkedList, 
              displaySimStartOrEnd, displayChangedState, allProcessesInState, 
              selectProcessToRun, displayProcessSelected, displayOpcodeLine
              runOpCmdForTime (run by thread), displayProcessEnd,
              displaySystemStop, displayOutputToFile, clearPCBLinkedList,
              clearFileOutLinkedList
*/
void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr);

/*
Name: selectProcessFCFSNSched
Process: selects the next process to execute using First-Come, 
         First-Served Non-Preemptive scheduling
Function Input/Parameters: PCBTypeLinkedList *pcbLinkedList
Function Output/Parameters: none
Function Output/Returned: pointer to the selected PCBType or NULL if none 
                          found
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: none
*/
PCBType *selectProcessFCFSNSched(PCBTypeLinkedList *pcbLinkedList);

/*
Name: selectProcessToRun
Process: selects a process to run based on the configured CPU scheduling 
         algorithm
Function Input/Parameters: ConfigDataType *configPtr, 
                           PCBTypeLinkedList *pcbLinkedList
Function Output/Parameters: none
Function Output/Returned: pointer to the selected PCBType
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: selectProcessFCFSNSched
*/
PCBType *selectProcessToRun(ConfigDataType *configPtr, 
                                              PCBTypeLinkedList *pcbLinkedList);

/*
Name: setProcessesToNewState
Process: initializes a linked list of PCBType nodes in NEW state based on 
         metadata commands and assigns process IDs
Function Input/Parameters: ConfigDataType *configPtr, 
                           OpCodeType *metaDataMstrPtr
Function Output/Parameters: none
Function Output/Returned: pointer to the initialized PCBTypeLinkedList
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: initializePCBLinkedList, insertPCBNode, calcTotalTime,
              compareString
*/
PCBTypeLinkedList *setProcessesToNewState(ConfigDataType *configPtr, 
                                                   OpCodeType *metaDataMstrPtr);

#endif // SIMULATOR_H