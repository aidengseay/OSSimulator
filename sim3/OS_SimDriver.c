// header files
#include "OS_SimDriver.h"

// main function

int main( int argc, char **argv)
{
    // initialize program

        // initialize variables
        ConfigDataType *configDataPtr = NULL;
        OpCodeType *metaDataPtr = NULL;
        char errorMessage[MAX_STR_LEN];
        CmdLineData cmdLineData;
        bool configUploadSuccess = false; 

        // show title
        printf("\nSimulator Program\n");
        printf("=================\n\n");

    // process command line, check for program run
    // at least one correct switch and config file name (at end) verified
    if(processCmdLine(argc, argv, &cmdLineData))
    {
        // upload config file, check for success
        if(getConfigData(cmdLineData.fileName, 
                                            &configDataPtr, errorMessage))
        {
            // check config display flag
            if(cmdLineData.configDisplayFlag)
            {
                // display config data
                displayConfigData(configDataPtr);
            }

            // set config upload success flag
            configUploadSuccess = true; 
        }

        // otherwise, assume config file upload failure
        else
        {
            // show error message, end program
            printf("\nConfig Upload Error: %s, program aborted\n\n",
                                                        errorMessage);
        }

        // check for config success and need for metadata
        if(configUploadSuccess 
                        &&(cmdLineData.mdDisplayFlag || cmdLineData.runSimFlag))
        {
            // upload meta data file, check for success
            if(getMetaData(configDataPtr->metaDataFileName,
                                                    &metaDataPtr, errorMessage))
            {
                // check meta data display flag
                if(cmdLineData.mdDisplayFlag)
                {
                    // display meta data
                    displayMetaData(metaDataPtr);
                }

                // check run simulator flag
                if(cmdLineData.runSimFlag)
                {
                    // run simulator
                    runSim(configDataPtr, metaDataPtr);
                }
            }

            // otherwise, assume meta data upload failure
            else
            {
            // show error message, end program
            printf("\nMetadata Upload Error: %s, program aborted\n", 
                                                                errorMessage);
            }
        }

        // clean up config data as needed
        configDataPtr = clearConfigData(configDataPtr);

        // clean up metadata as needed
        metaDataPtr = clearMetaDataList(metaDataPtr); 
    }

    // otherwise, assume command line failure
    else
    {
        // show command argument requirements
        showCommandLineFormat();
    }

    // show program end
    printf("\nSimulator Program End.\n\n");

    // return success
    return 0;
}

// supporting function implementations

////////////////////////////////////////////////////////////////////////////////
void clearCmdLineStruct(CmdLineData *clDataPtr)
{
    // set all struct members to default
    clDataPtr->programRunFlag = false;
    clDataPtr->configDisplayFlag = false;
    clDataPtr->mdDisplayFlag = false;
    clDataPtr->runSimFlag = false;
    clDataPtr->fileName[0] = NULL_CHAR;
}

////////////////////////////////////////////////////////////////////////////////
bool processCmdLine(int numArgs, char **strVector, CmdLineData *clDataPtr)
{
    // initialize function/variables

        // initialize structure to default
        clearCmdLineStruct(clDataPtr);
        
        // initialize success flags to false
        bool atLeastOneSwitchFlag = false;
        bool correctConfigFileName = false;

        // initialize first arg index to one
        int argIndex = 1; 

        // declare other variables
        int fileStrLen, fileStrSubLoc; 

    // must have prog name, at least 1 switch and config file name, min
    if(numArgs >= MIN_NUM_ARGS)
    {
        // loop across args (start @ 1) and program run flag
        while(argIndex < numArgs)
        {
            // check for -dc (display config flag)
            if(compareString(strVector[argIndex], "-dc") == STR_EQ)
            {
                // set config display flag
                clDataPtr->configDisplayFlag = true;

                // set at least one switch flag
                atLeastOneSwitchFlag = true;
            }

            // otherwise, check for -dm (display metadata flag)
            else if(compareString(strVector[argIndex], "-dm") == STR_EQ)
            {
                // set metadata display flag
                clDataPtr->mdDisplayFlag = true;

                // set at least one switch flag
                atLeastOneSwitchFlag = true;
            }

            // otherwise, check for -rs (run simulator)
            else if(compareString(strVector[argIndex], "-rs") == STR_EQ)
            {
                // set run simulator flag
                clDataPtr->runSimFlag = true;

                // set at least one switch flag
                atLeastOneSwitchFlag = true; 
            }

            // otherwise, check for file name, ending in .cfg
            // must be last four
            else
            {
                // find lengths to verify file name
                // file name must be last argument, and have .cfg extension
                fileStrLen = getStringLength(strVector[numArgs - 1]);
                fileStrSubLoc = findSubString(strVector[numArgs -1], ".cnf");

                // check for file existence and correct file format
                if(fileStrSubLoc != SUBSTRING_NOT_FOUND
                            && fileStrSubLoc == fileStrLen - LAST_FOUR_LETTERS)
                {
                    // set file name to variable
                    copyString(clDataPtr->fileName, strVector[numArgs - 1]);
                
                    // set success flag to true
                    correctConfigFileName = true;
                }

                // otherwise, assume bad config file name
                else
                {
                    // reset struct, correct config file, flag stays false
                    clearCmdLineStruct(clDataPtr);
                }
            }
            // update arg index
            argIndex++;
        }
    }
    // return verification of at least one switch and correct file name
    return atLeastOneSwitchFlag && correctConfigFileName;
}

////////////////////////////////////////////////////////////////////////////////
void showCommandLineFormat()
{
    // display command line format
    printf("Command Line Format:\n");
    printf("     sim_0x [-dc] [-dm] [-rs] <config file name>\n");
    printf("     -dc [optional] displays configuration data\n");
    printf("     -dm [optional] displays meta data\n");
    printf("     -rs [optional] runs simulator\n");
    printf("     required config file name\n");
}

////////////////////////////////////////////////////////////////////////////////