#include <stdio.h>

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
   sum = addend + augend;

   // display result to the user
      // function: printf
   printf("\nResult: %d\n\n", sum);

   // shut down
   return 0;
   }
