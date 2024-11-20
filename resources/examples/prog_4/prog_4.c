// header files
#include <stdio.h>
#include "StandardConstants.h"

// global constants

// function prototypes
int getStringLength( const char *testStr);

// main program

/*
Name: main
Process: finds the length of a string and displays it
Function Input/Parameters: none
Function Output/Parameters: none
Function Output/Returned: function/program success (0)
Device Input/Keyboard: none
Device Output/Monitor: result displayed
Dependencies: stdio tools
*/
int main()
    {
        // initialize function/variables

            // create a test string
            char testString[] = "This is a really cool string!";

            // declare other variables
            int strLen;

            // display title
            printf("\nString Length Test Program\n");
            printf("==========================\n\n");

        // conduct string length test
        strLen = getStringLength(testString);

        // display result to user
        printf("The length of string \'%s\' is %d\n\n", testString, strLen);

        // shut down program

            // display program end
            printf("\nProgram End\n");

            // return program success
            return 0;
    }


// function implementations

/*
Name: getStringLength
Process: finds the length of a string
         by counting characters up to the NULL_CHAR character
Function Input/Parameters: c-style string (char *)
Function Output/Parameters: none
Function Output/Returned: length of string (int)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: none
*/
int getStringLength( const char *testStr)
    {
        // initialize function/variables
        int index = 0;

        // loop to the end of string, protect from overflow
        while(index < STD_STR_LEN && testStr[index] != NULL_CHAR)

            // update index
            index ++;

        // end loop

        // return index/length
        return index;
    }