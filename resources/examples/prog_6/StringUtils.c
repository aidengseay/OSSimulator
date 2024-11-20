// header files
#include "StringUtils.h"

// local constants

// function implementations

/*
Name: compareString
Process: compares two strings with the following results:
         if left string less than right string, returns less than zero
         if left string greater than right string, returns greater than zero
         if left string equals right string, returns zero
         - equals test includes length
Function Input/Parameters: c-style left and right strings (char *)
Function Output/Parameters: none
Function Output/Returned: result as specified (int)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringLength
*/
int compareString( const char *oneStr, const char *otherStr)
    {
        // initialize function/variables
        int diff, index = 0; 

        // loop to end of shortest string w/ overrun protection
        while( oneStr[index] != NULL_CHAR
                                    && otherStr[index] != NULL_CHAR
                                                    && index < MAX_STR_LEN )
            {
                // get difference in characters
                diff = oneStr[index] - otherStr[index];

                // check for difference between characters
                if(diff != 0)
                    {
                        // return difference
                        return diff; 
                    }

                // increment index
                index ++; 
            }

        // return difference in lengths, if any
        return getStringLength(oneStr) - getStringLength(otherStr);
    }

/*
Name: concatenateString
Process: appends one string onto another
Function Input/Parameters: c-style source string (char *)
Function Output/Parameters: c-style destination string (char *)
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringLength
*/
void concatenateString(char *destStr, const char *sourceStr)
    {
        // initialize function/variables

            // set destination index
            int destIndex = getStringLength(destStr);

            // get source string length
            int sourceStrLen = getStringLength(sourceStr);

            // create temporary string pointer
            char *tempStr;

            // create other variables
            int sourceIndex = 0; 

        // copy source string in case of aliasing
        tempStr = (char*)malloc(sourceStrLen + 1);
        copyString(tempStr, sourceStr);

        // loop to the end of source string
        while(tempStr[sourceIndex] != NULL_CHAR && destIndex < MAX_STR_LEN)
            {
                // assign character to end of destination string
                destStr[destIndex] = tempStr[sourceIndex];

                // update indices
                destIndex ++; sourceIndex ++;

                // set temporary end of destination string
                destStr[destIndex] = NULL_CHAR;
            }

        // release memory used for temp string
        free(tempStr);
    }

/*
Name: copyString
Process: copies one string into another,
         overwriting data in the destination string
Function Input/Parameters: c-style source string (char *)
Function Output/Parameters: c-style destination string (char *)
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringLength
*/
void copyString(char *destStr, const char *sourceStr)
    {
        // initialize function/variables
        int index = 0;

        // check for source/dest not the same (aliasing)
        if(destStr != sourceStr)
            {
                // loop to end of source string
                while(sourceStr[index] != NULL_CHAR && index < MAX_STR_LEN) 
                    {
                        // assign characters to end of destination string
                        destStr[index] = sourceStr[index];

                        // update index
                        index ++;

                        // set temporary end of destination string
                        destStr[index] = NULL_CHAR;
                    }   
            }
    }

/*
Name: findSubString
Process: linear search for given substring within another string
Function Input/Parameters: c-style source test string (char *)
                           c-style source search string (char *)
Function Output/Parameters: none
Function Output/Returned: index of found substring, or
                          SUBSTRING_NOT_FOUND constant
                          if string not found
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringLength
*/
int findSubString(const char *testStr, const char *searchSubStr)
    {
        // initialize function/variables

            // initialize test string length
            int testStrLen = getStringLength(testStr);

            // initialize master index = location of sub string start point
            int masterIndex = 0;

            // initialize other variables
            int searchIndex, internalIndex;

        // loop across test string
        while(masterIndex <testStrLen)
            {
                // set internal loop index to current test string index
                internalIndex = masterIndex;

                // set internal seach index to zero
                searchIndex = 0; 

                // loop to end of test string
                // while test string/sub string characters are the same
                while(internalIndex <= testStrLen && testStr[internalIndex] == 
                                                      searchSubStr[searchIndex])
                    {
                        // increment test string, substring indices
                        internalIndex++; searchIndex++;

                        // check for end of substring (search completed)
                        if(searchSubStr[searchIndex] == NULL_CHAR)
                            {
                                return masterIndex;
                            }
                    }

                // increment current beginning location index
                masterIndex++;
            }

        // assume test have failed at this point, return SUBSTRING_NOT_FOUND
        return SUBSTRING_NOT_FOUND;
    }

/*
Name: getStringConstrained
Process: captures a string from the input stream
         with various constraints
Function Input/Parameters: input stream (FILE *)
                           clears leading non printable (bool)
                           clears leading space (bool)
                           stops at non printable (bool)
                           stops at specified delimiter (char)
                           Note: consumes delimiter
Function Output/Parameters: string returned (char *)
Function Output/Returned: success of operation (bool)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: fgetc
*/
bool getStringConstrained(
                          FILE *inStream,
                          bool clearLeadingNonPrintable,
                          bool clearLeadingSpace,
                          bool stopAtNonPrintable,
                          char delimiter,
                          char *capturedString)
    {
        // initialize function/variables
        int intChar = EOF, index = 0;

        // initialize output string
        capturedString[index] = NULL_CHAR;

        // capture first value in stream
        intChar = fgetc(inStream);

        // loop to clean non printable or space, if indicated
        while((intChar != EOF) 
                    && ((clearLeadingNonPrintable && intChar < (int)SPACE)
                                ||(clearLeadingSpace && intChar == (int)SPACE)))
            {
               // get next character
               intChar = fgetc(inStream);
            }   

        // check for end of file found
        if(intChar == EOF)
            {
                // return failed operation
                return false;
            }

        // loop to capture input
        while(
            // continues if not at end of file and max string length is not reached
            (intChar != EOF && index < MAX_STR_LEN - 1)

            // AND
            // continues if not printable flag set and characters are printable
            // OR continues if not printable flag not set
            && ((stopAtNonPrintable && intChar >= (int)SPACE)
                    || (!stopAtNonPrintable))

            // AND
            // continues if specified delimiter is not found
            && (intChar != (int)delimiter))
            {
                // place character in array element
                capturedString[index] = (char)intChar;

                // increment array index
                index++; 

                // set next element to null character / end of c-string
                capturedString[index] = NULL_CHAR;

                // get next character as integer
                intChar = fgetc(inStream);
            }

        // return successful operation
        return true;
    }


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
int getStringLength(const char *testStr)
    {
        // initialize function/variables
        int index = 0;

        // loop to the end of string, protect from overflow
        while(index < STD_STR_LEN && testStr[index] != NULL_CHAR)

            // update index
            index ++;

        // return index/length
        return index;
    }

/*
Name: getStringToDelimiter
Process: captures a string from the input stream
         to a specified delimiter;
         Note: consumes delimiter
Function Input/Parameters: input stream (FILE *)
                           stops at specified delimiter (char)
Function Output/Parameters: string returned (char *)
Function Output/Returned: success of operation (bool)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringConstrained
*/
bool getStringToDelimiter(FILE *inStream, char delimiter, char *capturedString)
    {
        // call engine function with delimiter
        return getStringConstrained(
            inStream,               // file stream pointer
            true,                   // clears leading non printable character
            true,                   // bool clearLeadingSpace 
            true,                   // stops at non printable  
            delimiter,              // stops at delimiter
            capturedString          // returns string
                        );
    }

/*
Name: getStringToLineEnd
Process: captures a string from the input stream
         to the end of the line;
Function Input/Parameters: input stream (FILE *)
Function Output/Parameters: string returned (char *)
Function Output/Returned: success of operation (bool)
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringConstrained
*/
bool getStringToLineEnd(FILE *inStream, char *capturedString)
    {
        // call engine function with delimiter
        return getStringConstrained(
                inStream,               // file stream pointer
                true,                   // clears leading non printable char
                true,                   // bool clearLeadingSpace
                true,                   // stops at non printable
                NON_PRINTABLE_CHAR,     // non printable delimiter
                capturedString          // returns string
                                ); 
        
    }

/*
Name: getSubString
Process: captures sub string within larger string
         between two inclusive indicies.
         returns empty string if either index is out of range,
         assumes enough memory in destination string
Function Input/Parameters: c-style source string (char *),
                           start and end indices (int)
Function Output/Parameters: c-style destination string (char *)
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: getStringLength, malloc, copyString, free
*/
void getSubString(char *destStr, const char *sourceStr, 
                        int startIndex, int endIndex)
    {
        // initialize function/variables

            // set length of source string
            int sourceStrLen = getStringLength(sourceStr);

            // initialize the dest index to 0
            int destIndex = 0;

            // initialize source index to start index (parameter)
            int sourceIndex = startIndex;

            // create pointer for temp string
            char *tempStr;

        // check for indices within limits
        if(startIndex >= 0 && startIndex <= endIndex && endIndex < sourceStrLen)
            {
                // create temp string
                tempStr = (char*)malloc(sourceStrLen + 1);
                copyString(tempStr, sourceStr);

                // loop across requested substring (indices)
                while(sourceIndex <= endIndex)
                    {
                        // assign source character to destination element
                        destStr[destIndex] = tempStr[sourceIndex];

                        // increment indices
                        destIndex++; sourceIndex++;

                        // set temp end of dest string
                        destStr[destIndex] = NULL_CHAR;
                    }

                // return memory for temp string
                free(tempStr);
            }
    }

/*
Name: setStrToLowerCase
Process: iterates though string, sets any upper case letter
         to lower case; no effect on other letters
Function Input/Parameters: c-style source string (char *)
Function Output/Parameters: c-style destination string (char *)
Function Output/Returned: none
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: toLowerCase
*/
void setStrToLowerCase(char *destStr, const char *sourceStr)
    {
        // initialize function/variables

            // get source string length
            int sourceStrLen = getStringLength(sourceStr);

            // create a temp string pointer
            char *tempStr;

            // create other variables
            int index = 0; 

        // copy source string in case of aliasing
        tempStr = (char*)malloc(sourceStrLen + 1);
        copyString(tempStr, sourceStr);

        // loop across source string
        while(tempStr[index] != NULL_CHAR && index < MAX_STR_LEN)
            {
                // set individual character to lower case as needed,
                // assign to dest string
                destStr[index] = toLowerCase(tempStr[index]);

                // update index
                index++;

                // set temp end of dest string
                destStr[index] = NULL_CHAR;
            }

        // release memory used for temp string
        free(tempStr);
    }

/*
Name: toLowerCase
Process: if character is upper case, sets it to lower case;
         otherwise returns character unchanged
Function Input/Parameters: test character (char)
Function Output/Parameters: none
Function Output/Returned: character to set to lowercase, if appropriate
Device Input/Keyboard: none
Device Output/Monitor: none
Dependencies: none
*/
char toLowerCase(char testChar)
    {
        // check for uppercase letter
        if(testChar >= 'A' && testChar <= 'Z')
        {
            // return lowercase letter
            return testChar -'A' + 'a';
        }

        // otherwise, assume no uppercase letter,
        // return charater unchanged
        return testChar;
    }