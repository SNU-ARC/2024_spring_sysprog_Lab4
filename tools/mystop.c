/*
 * mystop.c - Another handy routine for testing your shell
 *
 * usage: mystop <n>
 * Sleeps for <n> seconds and sends SIGTSTP to itself.
 * Prints a number every second to stdout in 1-second intervals.
 *
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
  pid_t pid;

  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <n>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  secs = atoi(argv[1]);

  for (i=0; i < secs; i++) {
    sleep(1);

    fprintf(stdout, "%d\n", i+1);
  }

  pid = getpid();

  if (kill(-pid, SIGTSTP) < 0) fprintf(stderr, "kill (tstp) error");

  exit(EXIT_SUCCESS);
}
