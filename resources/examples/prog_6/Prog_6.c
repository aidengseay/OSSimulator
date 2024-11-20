// header files
#include "StringUtils.h"

// constants

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

            // create test strings
            char testString[] = "This is a really cool string!";
            char otherString[MAX_STR_LEN];
            char subString[MAX_STR_LEN];
            char lowerCaseString[MAX_STR_LEN];
            char captureString[MAX_STR_LEN];

            // declare other variables
            int strLen, subStrStart; 

            // display title
            printf("\nString Length Test Program\n");
            printf("==========================\n\n");

        // conduct string length test
        strLen = getStringLength(testString);

        // display length test to user
        printf("The length of string \'%s\' is %d\n\n", testString, strLen);

        // test copy string function
        copyString(otherString, testString);

        // display copy test to user
        printf("Copied string is: %s\n\n", otherString);

        // test concatenate string function
        concatenateString(otherString, " I'm pretty sure of it.");

        // display concatenate test to user
        printf("Concatenated string is: %s\n\n", otherString);

        // test compare string function for greater than condition
        if(compareString("Sally", "Roger") > STR_EQ)
        {
            // display compare/greater than test to user
            printf("\'Sally\' is greater than \'Roger\'\n\n");
        }

        // test compare string function for less than condition
        if(compareString("Cindy", "Sally") < STR_EQ)
        {
            // display compare/less than test to user
            printf("\'Cindy\' is less than \'Sally\'\n\n");
        }

        // test compare string function for equals condition
        if(compareString("Benedict Cumberbatch","Benedict Cumberbatch") == STR_EQ)
        {
            // display equals test to user
            printf("\'Benedict Cumberbatch\' is equal to \'Benedict Cumberbatch\'\n\n");
        }

        // test get sub string function
        getSubString( subString, otherString, 34, 44);

        // display get sub string test to user
        printf("Sub string found: %s\n\n", subString);

        // test find sub string function
        subStrStart = findSubString(otherString, "pretty sure");

        // display find sub string test to user
        printf("Sub string start: %d\n\n", subStrStart);

        // test for setting string with any upper case letter(s)
        // to lower case
        setStrToLowerCase(lowerCaseString, otherString);

        // display lower case string result to user
        printf("Lower case string: %s\n\n", lowerCaseString);

        // test get line to delimiter
        getStringToDelimiter(stdin, '.', captureString);

        // display get line to delimiter string result to user
        printf("Captured to delimiter: %s\n\n", captureString);

        // test get line to end of line
        getStringToLineEnd(stdin, captureString);

        // display get line to line end result to user
        printf("captured to end of line     : %s\n\n", captureString);

        // shut down program

            // display program end
            printf("End Program\n");

            // return program success
            return 0;
    }