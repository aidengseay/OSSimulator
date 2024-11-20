// header files
#include "StringUtils.h"

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