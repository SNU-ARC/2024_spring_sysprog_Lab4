/*
 * myspin.c - A handy program for testing your shell
 *
 * usage: myspin <n>
 * Sleeps for <n> seconds in 1-second chunks.
 * Prints a line every second to stdout in 1-second intervals.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  int i, secs;
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <n>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  secs = atoi(argv[1]);

  for (i=0; i < secs; i++) {
    sleep(1);
    fprintf(stdout, "spinning\n");
  }

  exit(EXIT_SUCCESS);
}
