// header files
#include "Configops.h"

// function implementations

////////////////////////////////////////////////////////////////////////////////
ConfigDataType *clearConfigData(ConfigDataType *configData)
{
    // check that config data pointer is not NULL
    if(configData != NULL)
    {
        // free data structure memory
        free(configData);

        // set config pointer to NULL
        configData = NULL;
    }

    // return NULL config data pointer
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
void configCodeToString(int code, char *outString)
{
    // define array with eight items, and short (10) lengths
    char displayStrings[8][10] = {"SJF-N", "SRTF-P", "FCFS-P",
                                  "RR-P", "FCFS-N", "Monitor",
                                  "File", "Both"};

    // copy string to return parameter
    copyString(outString, displayStrings[code]);
}

////////////////////////////////////////////////////////////////////////////////
void displayConfigData(ConfigDataType *configData)
{
    // initialize function/variables
    char displayString[STD_STR_LEN];

    // print lines of display for all member values
    printf("Config File Display\n");
    printf("-------------------\n");
    printf("Version               : %3.2f\n", configData->version);
    printf("Program file name     : %s\n", configData->metaDataFileName);
    configCodeToString(configData->cpuSchedCode, displayString);
    printf("CPU schedule selection: %s\n", displayString);
    printf("Quantum time          : %d\n", configData->quantumCycles);
    printf("Memory Display        : ");
    if(configData->memDisplay)
    {
        printf("On\n");
    }
    else
    {
        printf("Off\n");
    }
    printf("Memory available      : %d\n", configData->memAvailable);
    printf("Process cycle rate    : %d\n", configData->procCycleRate);
    printf("I/O cycle rate        : %d\n", configData->ioCycleRate);
    configCodeToString(configData->logToCode, displayString);
    printf("Log to selection      : %s\n", displayString);
    printf("Log file name         : %s\n\n", configData->logToFileName);
}

////////////////////////////////////////////////////////////////////////////////
bool getConfigData(const char *filename,
                                ConfigDataType **configData, char *endStateMsg)
{
    // initialize function/variables

        // set constant number of data lines
        const int NUM_DATA_LINES = 10;

        // read only constant
        const char READ_ONLY_FLAG[] = "r";

        // creat pointer for data input
        ConfigDataType *tempData; 

        // declare other variables
        FILE *fileAccessPtr;
        char dataBuffer[MAX_STR_LEN], lowerCaseDataBuffer[MAX_STR_LEN];
        int intData, dataLineCode, lineCtr = 0;
        double doubleData; 

    // set endStateMsg to success
    char testMsg[] = "Configuration file upload successful";
    copyString(endStateMsg, testMsg);

    // initialize config data pointer in case of return error
    *configData = NULL;

    // open file
    fileAccessPtr = fopen(filename, READ_ONLY_FLAG);

    // check for file open failure
    if(fileAccessPtr == NULL)
    {
        // set end state message to config file access error
        char testMsg[] = "Configuration file access error";
        copyString(endStateMsg, testMsg);

        // return file access error
        return false;
    }

    // get first line, check for failure
    if(!getStringToDelimiter(fileAccessPtr, COLON, dataBuffer)
       || compareString(dataBuffer, 
                                "Start Simulator Configuration File") != STR_EQ)
    {
        // close file access
        fclose(fileAccessPtr);

        // set end state message to corrupt leader line error
        copyString(endStateMsg, "Corrupt configuration leader line error");

        // return corrupt file data
        return false;
    }

    // create temporay pointer to config data structure
    tempData = (ConfigDataType *)malloc(sizeof(ConfigDataType));

    // loop to end of config data items
    while(lineCtr < NUM_DATA_LINES)
    {
        // get line leader, check for failure
        if(!getStringToDelimiter(fileAccessPtr, COLON, dataBuffer))
        {
            // free temp struct memory
            free(tempData);

            // close file access
            fclose(fileAccessPtr);

            // set end state message to line capture failure
            copyString(endStateMsg,
                                "Configuration data leader line capture error");

            // return incomplete file error
            return false; 
        }

        // strip training spaces for config leader lines
        // for use in other functions
        stripTrailingSpaces(dataBuffer);

        // find correct data line code number from string
        dataLineCode = getDataLineCode(dataBuffer);

        // check for data line found
        if(dataLineCode != CFG_CORRUPT_PROMPT_ERR)
        {
            // get data value

                // check for version number (double value)
                if(dataLineCode == CFG_VERSION_CODE)
                {
                    // get version number
                    fscanf(fileAccessPtr, "%lf", &doubleData);
                }

                // otherwise, check for metaData, file names, CPU
                // scheduling names, or memory display setting (strings)
                else if(dataLineCode == CFG_MD_FILE_NAME_CODE
                        || dataLineCode == CFG_LOG_FILE_NAME_CODE
                        || dataLineCode == CFG_CPU_SCHED_CODE
                        || dataLineCode == CFG_LOG_TO_CODE
                        || dataLineCode == CFG_MEM_DISPLAY_CODE)
                {
                    // get string input
                    fscanf(fileAccessPtr, "%s", dataBuffer);

                    // set string to lowercase for testing in valueInRange
                    setStrToLowerCase(lowerCaseDataBuffer, dataBuffer);
                }

                // otherwise, assume integer data
                else
                {
                    // get integer input
                    fscanf(fileAccessPtr, "%d", &intData);
                }

            // check for data value in range
            if(valueInRange(dataLineCode, intData,
                                               doubleData, lowerCaseDataBuffer))
            {
                // assign to data pointer depending on config item
                // (all config line possibilities)
                switch(dataLineCode)
                {
                    case CFG_VERSION_CODE:
                        tempData->version = doubleData;
                        break;

                    case CFG_MD_FILE_NAME_CODE:
                        copyString(tempData->metaDataFileName, dataBuffer);
                        break;

                    case CFG_CPU_SCHED_CODE:
                        tempData->cpuSchedCode 
                                         = getCpuSchedCode(lowerCaseDataBuffer);
                        break; 

                    case CFG_QUANT_CYCLES_CODE:
                        tempData->quantumCycles = intData;
                        break;

                    case CFG_MEM_DISPLAY_CODE:
                        tempData->memDisplay
                           = compareString(lowerCaseDataBuffer, "on") == STR_EQ;
                        break;

                    case CFG_MEM_AVAILABLE_CODE:
                        tempData->memAvailable = intData;
                        break;

                    case CFG_PROC_CYCLES_CODE:
                        tempData->procCycleRate = intData;
                        break;

                    case CFG_IO_CYCLES_CODE:
                        tempData->ioCycleRate = intData;
                        break;

                    case CFG_LOG_TO_CODE:
                        tempData->logToCode = getLogToCode(lowerCaseDataBuffer);
                        break;

                    case CFG_LOG_FILE_NAME_CODE:
                        copyString(tempData->logToFileName, dataBuffer);
                        break;
                }
            }

            // otherwise, assume data value not in range
            else
            {
                // free temp struct memory
                free(tempData);

                // close file access
                fclose(fileAccessPtr);

                // set end state message to config data out of range
                copyString(endStateMsg, "Configuration item out of range");

                // return data out of range
                return false;
            }
        }

        // otherwise, assume data line not found
        else
        {
            // free temp struct memory
            free(tempData);

            // close file access
            fclose(fileAccessPtr);

            // sent end state message to config corrupt prompt error
            copyString(endStateMsg, "Corrupted configuration prompt");

            // return corrupt config file code
            return false;
        }

        // increment line counter
        lineCtr++;
    }

    // acquire end of sim config string
    if(!getStringToDelimiter(fileAccessPtr, PERIOD, dataBuffer)
            || compareString(dataBuffer, 
                                  "End Simulator Configuration File") != STR_EQ)
    {
        // free temp struct memory
        free(tempData);

        // close file access
        fclose(fileAccessPtr);

        // set end state message to corrupt config end line
        copyString(endStateMsg, "Configuration end line capture error");

        // return corrupt file data
        return false;
    }

    // test for "file only" output so memory diagnostics do not display
    tempData->memDisplay = tempData->memDisplay
                                      && tempData->logToCode != LOGTO_FILE_CODE;

    // assign temp pointer to parameter return pointer
    *configData = tempData; 

    // close file access
    fclose(fileAccessPtr);

    // return no error code
    return true;
}

////////////////////////////////////////////////////////////////////////////////
ConfigDataCodes getCpuSchedCode(const char *lowerCaseCodeStr)
{
    // initialize function/variables

        // set default return to FCFS-N
        ConfigDataCodes returnVal = CPU_SCHED_FCFS_N_CODE;

    // check for SJF-N
    if(compareString(lowerCaseCodeStr, "sjf-n") == STR_EQ)
    {
        // set SJF-N code
        returnVal = CPU_SCHED_SJF_N_CODE;
    }

    // check for SRTF-P
    if(compareString(lowerCaseCodeStr, "srtf-p") == STR_EQ)
    {
        // set SRTF-P code
        returnVal = CPU_SCHED_SRTF_P_CODE;
    }

    // check for FCFS-P
    if(compareString(lowerCaseCodeStr, "fcfs-p") == STR_EQ)
    {
        // set FCFS-P code
        returnVal = CPU_SCHED_FCFS_P_CODE;
    }

    // check for RR-P
    if(compareString(lowerCaseCodeStr, "rr-p") == STR_EQ)
    {
        // set RR-P code
        returnVal = CPU_SCHED_RR_P_CODE;
    }

    // return the selected value
    return returnVal;
}

////////////////////////////////////////////////////////////////////////////////
ConfigCodeMessages getDataLineCode(const char *dataBuffer)
{
    // return appropiate code depending on prompt string provided
    if(compareString(dataBuffer, "Version/Phase") == STR_EQ)
    {
        return CFG_VERSION_CODE;
    }

    if(compareString(dataBuffer, "File Path") == STR_EQ)
    {
        return CFG_MD_FILE_NAME_CODE;
    }

    if(compareString(dataBuffer, "CPU Scheduling Code") == STR_EQ)
    {
        return CFG_CPU_SCHED_CODE;
    }

    if(compareString(dataBuffer, "Quantum Time (cycles)") == STR_EQ)
    {
        return CFG_QUANT_CYCLES_CODE;
    }

    if(compareString(dataBuffer, "Memory Display (On/Off)") == STR_EQ)
    {
        return CFG_MEM_DISPLAY_CODE;
    }

    if(compareString(dataBuffer, "Memory Available (KB)") == STR_EQ)
    {
        return CFG_MEM_AVAILABLE_CODE;
    }

    if(compareString(dataBuffer, "Processor Cycle Time (msec)") == STR_EQ)
    {
        return CFG_PROC_CYCLES_CODE;
    }

    if(compareString(dataBuffer, "I/O Cycle Time (msec)") == STR_EQ)
    {
        return CFG_IO_CYCLES_CODE;
    }

    if(compareString(dataBuffer, "Log To") == STR_EQ)
    {
        return CFG_LOG_TO_CODE;
    }

    if(compareString(dataBuffer, "Log File Path") == STR_EQ)
    {
        return CFG_LOG_FILE_NAME_CODE;
    }

    // return corrupt leader line error code
    return CFG_CORRUPT_PROMPT_ERR;
}

////////////////////////////////////////////////////////////////////////////////
ConfigDataCodes getLogToCode(const char *lowerCaseLogToStr)
{
    // initialize function/variables

        // set default to log to monitor
        ConfigDataCodes returnVal = LOGTO_MONITOR_CODE;

    // check for BOTH
    if(compareString(lowerCaseLogToStr, "both") == STR_EQ)
    {
        // set return value to both code
        returnVal = LOGTO_BOTH_CODE;
    }

    // check for FILE
    if(compareString(lowerCaseLogToStr, "file") == STR_EQ)
    {
        // set return value to file code
        returnVal = LOGTO_FILE_CODE;
    }

    // return selected code
    return returnVal;
}

////////////////////////////////////////////////////////////////////////////////
void stripTrailingSpaces(char *str)
{
    // initialize index to length of string - 1 to get highest array index
    int index = getStringLength(str) - 1;

    // loop while space is found at end of string
    while(str[index] == SPACE)
    {
        // set element to NULL_CHAR
        str[index] = NULL_CHAR;

        // decrement index
        index--;
    }
}

////////////////////////////////////////////////////////////////////////////////
bool valueInRange(int lineCode, int intVal, double doubleVal, 
                                                 const char *lowerCaseStringVal)
{
    // initialize function/ variables

        // set result to true, all tests are to find false
        bool result = true; 

    // use line code to id prompt line
    switch(lineCode)
    {
        // for version code
        case CFG_VERSION_CODE:

            // check if limits of version code are exceeded
            if(doubleVal < 0.00 || doubleVal > 10.00)
            {
                // set boolean result to false
                result = false;
            }

        break;

        // for cpu scheduling code
        case CFG_CPU_SCHED_CODE:

            // check for not finding one of the scheduling strings
            if(compareString(lowerCaseStringVal, "fcfs-n") != STR_EQ
                && compareString(lowerCaseStringVal, "sjf-n") != STR_EQ
                && compareString(lowerCaseStringVal, "srtf-p") != STR_EQ
                && compareString(lowerCaseStringVal, "fcfs-p") != STR_EQ
                && compareString(lowerCaseStringVal, "rr-p") != STR_EQ)
            {
                // set boolean result to false
                result = false;
            }

        break;

        // for quantum cycles
        case CFG_QUANT_CYCLES_CODE:

            // check for quantum cycles limits exceeded
            if(intVal < 0 || intVal > 100)
            {
                // set boolean result to false
                result = false;
            }

        break; 

        // for memory display
        case CFG_MEM_DISPLAY_CODE:

            // check for not finding either "on" or "off"
            if(compareString(lowerCaseStringVal, "on") != STR_EQ
                && compareString(lowerCaseStringVal, "off") != STR_EQ)
            {
                // set boolean result to false
                result = false; 
            }

        break;

        // for memory available
        case CFG_MEM_AVAILABLE_CODE:

            // check for available memory limits exceeded
            if( intVal < 1024 || intVal > 102400)
            {
                // set boolean result to false
                result = false; 
            }

        break;

        // check for process cycles
        case CFG_PROC_CYCLES_CODE:

            // check for process cycles limits exceeded
            if(intVal < 1 || intVal > 100)
            {
                // set boolean result to false
                result = false; 
            }

        break;

        //  check for IO cycles
        case CFG_IO_CYCLES_CODE:

            // check for IO cycles limits exceeded
            if(intVal < 1 || intVal > 1000)
            {
                // set boolean result to false
                result = false; 
            }

        break;

        // check for log to operation
        case CFG_LOG_TO_CODE:

            // check for not finding one of the log to strings
            if(compareString(lowerCaseStringVal, "both") != STR_EQ
                && compareString(lowerCaseStringVal, "monitor") != STR_EQ
                && compareString(lowerCaseStringVal, "file") != STR_EQ)
            {
                // set boolean result to false
                result = false; 
            }

        break;
    }

    // return result of limits analysis
    return result;
}

////////////////////////////////////////////////////////////////////////////////