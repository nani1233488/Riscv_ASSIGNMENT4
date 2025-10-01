#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("Starting COW test...\n");

  // Allocate one page of memory
  char *mem = sbrk(4096);
  if (mem == (char*)-1) {
    fprintf(2, "sbrk failed\n");
    exit(1);
  }

  // Write an initial value to the page
  *mem = 'A';
  printf("Parent allocated a page and wrote 'A' to it.\n");

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process
    printf("Child process started.\n");
    
    // The child's write should trigger a page fault and a copy.
    printf("Child writing 'B' to the shared page.\n");
    *mem = 'B'; 
    
    printf("Child reads '%c' from its page.\n", *mem);
    
    exit(0);
  } else {
    // Parent process
    wait(0);
    printf("Child has finished.\n");
    
    // The parent must see the original value.
    printf("Parent reads '%c' from its page.\n", *mem);
    
    if (*mem == 'A') {
      printf("\nSUCCESS: Parent's memory is unchanged.\n");
      printf("COW test passed!\n");
    } else {
      printf("\nFAILURE: Parent's memory was modified.\n");
      printf("COW test failed!\n");
    }
  }

  exit(0);
}