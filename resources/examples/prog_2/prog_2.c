#include <stdio.h>

/*
Function Name: addValues
Algorithm: accept two int values, add them too a double sum,
           then returns a double sum
Preconditions: two values as ints
Postcondition: return int sum as specified
Exceptions: none
Note: demonstrate creation of supporting function
*/
int addValues(int oneVal, int otherVal)
{
    return oneVal + otherVal;
}

/*
Function Name: main
Algorithm: accept two int values, add them too a double sum,
           then display results
Preconditions: none
Postcondition: return 0 on success
Exceptions: none
Note: demonstrate input, output, and simple math
*/

int main()
   {
    // initialize function. variables
    int augend, addend, sum; 

    // display title
        // function: printf
   printf("\nAddition Program\n");
   printf("================\n");


    // get input from the user

        // get augend
            // function: printf, scanf
      printf("\nEnter augend: ");
      scanf("%d", &augend);

        // get addend
            // function: printf, scanf
      printf("\nEnter addend: ");
      scanf("%d", &addend);

    // add values
    sum = addValues(augend, addend);

    // display result to the user
        // function: printf
   printf("\nResult: %d\n\n", sum);

   // shut down
   return 0;
   }
