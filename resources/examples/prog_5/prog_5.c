// header files
#include <stdio.h>
#include "StringUtils.h"

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