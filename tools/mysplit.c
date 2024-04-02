/*
 * mysplit.c - Another handy routine for testing your shell
 *
 * usage: mysplit <n>
 * Fork a child that spins for <n> seconds in 1-second chunks.
 * The child prints a line every second to stdout in 1-second intervals.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char *argv[])
{
  int i, secs;
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <n>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  secs = atoi(argv[1]);


  if (fork() == 0) {
    // child
    for (i=0; i < secs; i++) {
      sleep(1);

      fprintf(stdout, "Child!\n");
    }
    exit(EXIT_SUCCESS);
  }

  // parent waits for child to terminate
  wait(NULL);

  exit(EXIT_SUCCESS);
}
