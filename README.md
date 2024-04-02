# Lab 4: Shell Lab

The purpose of this lab is to become more familiar with the concepts of process control, signaling, pipes, and redirection. 
To achieve that goal, we implement our own shell with simple job control.

You will learn
   * how to run other processes through `fork()` and `exec()`
   * how to redirect input/output
   * how to create a pipe between two child processes
   * how to send a group of processes to the background/foreground
   * that proper signal handling is quite tricky


## Important Dates

| Date | Description |
|:---  |:--- |
| Tuesday, May 14, 18:30 | Hand-out |
| Tuesday, May 21, 18:30 | Lab session 1 |
| Tuesday, May 28, 18:30 | Lab session 2 |
| Monday, June 03, 23:59 | Submission deadline |


## Logistics

### Hand-out
You can clone this repository directly on your VM instance or local computer and get to work. If you want to keep your own repository, you should keep the lab's visibility to private. Otherwise, others would see your work. Read the instructions here carefully. Then clone the lab to your local computer and get to work.

#### Changing visibility
After cloning the repository, you should change the push remote URL to your own repository.
1. Create an empty repository that you're going to manage (again, keep it private)
2. Copy the url of that repository
3. On your terminal in the cloned directory, type `git remote set-url --push origin <repo_url>`
4. Check with `git remote -v` if the push URL has changed to yours while the fetch URL remains the same (this repo)


### Submission

You should upload your archive file(.tar) containing code (csapsh.c) and report (2024-12345.pdf) via eTL. To make an archive file, follow the example below on your own VM.
```
$ ls
2024-12345.pdf  csapsh  Makefile  obj  README.md  reference  src  tools  traces
$ tar -cvf 2024-12345.tar src/csapsh.c 2024-12345.pdf
src/csapsh.c
2024-12345.pdf
$ file 2024-12345.tar
2024-12345.tar: POSIX tar archive (GNU)
```
- With VirtualBox :
```bash
scp -P 8888 sysprog@localhost:<target_path> <download_path>
# example: scp -P 8888 sysprog@localhost:/home/sysporg/2024_spring_sysprog_Lab4/2024-12345.tar .
```
- With UTM :
```bash
scp sysprog@<hostname>:<target_path> <download_path>
# example: scp sysprog@192.168.4.4:/home/sysprog/2024_spring_sysprog_Lab4/2024-12345.tar .
```

| parameter | Description |
|:---  |:--- |
| hostname | ip address of VM |
| target_path | absolute path of the file you want to copy (in VM) |
| download_path | (relative) path where a file will be downloaded (in PC) |

You can get your VM's hostname using `hostname -I` command, and the absolute path of a file using `realpath <filename>` command.

#### Report Guideline

Your report should include following contents.

1. Description of your implementation
2. Difficulties and thoughts during the implementation of this lab


## The CSAP Shell

### Overview

A shell is an interactive command-line interpreter that runs programs on behalf of the user. A shell repeatedly prints a prompt, waits for a command line on stdin, and then carries out some action, as directed by the contents of the command line.

The command line is a sequence of ASCII text words delimited by whitespace. The first word in the command line is either the name of a built-in command or the pathname of an executable file. The remaining words are command-line arguments. If the first word is a built-in command, the shell immediately executes the command in the current process. Otherwise, the word is assumed to be the pathname of an executable program. In this case, the shell forks a child process, then loads and runs the program in the context of the child. The child processes created as a result of interpreting a single command line are known collectively as a job. In general, a job can consist of multiple child processes connected by Unix pipes.

If the command line ends with an ampersand "&", then the job runs in the background, which means that the shell does not wait for the job to terminate before printing the prompt and awaiting the next command line. Otherwise, the job runs in the foreground, which means that the shell waits for the job to terminate before awaiting the next command line. Thus, at any point in time, at most one job can be running in the foreground. However, an arbitrary number of jobs can run in the background.

For example, typing the command line

~~~bash
csapsh> jobs
~~~

causes the shell to execute the built-in `jobs` command. Typing the command line

~~~bash
csapsh> ls -l -d /etc
~~~

runs the `ls` program in the foreground. By convention, the shell ensures that when a program begins executing its main routine

```C
int main(int argc, char *argv[])
 ```

the `argc` and `argv` arguments have the following values:

* `argc`: number of command line arguments, including program name
* `argv`: NULL-terminated array of `char*` that hold the command line arguments starting with the program name in `argv[0]`

For the `ls -l -d /etc` example above, `argc` and `argv` contain the values
* `argc`: 4
* `argv`:
    * `argv[0]`: `ls`
    * `argv[1]`: `-l`
    * `argv[2]`: `-d`
    * `argv[3]`: `/etc`
    * `argv[4]`: NULL

If the command line is terminated with an ampersand character (&), the command is run in the background:
~~~bash
csapsh> ls -l -d /etc &
~~~

Unix shells support the notion of job control, which allows users to move jobs back and forth between background and foreground, and to change the process state (running, stopped, or terminated) of the processes in a job. Typing ctrl-c causes a SIGINT signal to be delivered to each process in the foreground job. The default action for SIGINT is to terminate the process. Similarly, typing ctrl-z causes a SIGTSTP signal to be delivered to each process in the foreground job. The default action for SIGTSTP is to place a process in the stopped state, where it remains until it is awakened by the receipt of a SIGCONT signal. Unix shells also provide various built-in commands that support job control. For example:

* `jobs`: List the running and stopped background jobs.
* `bg <job>`: Change a stopped background job to a running background job.
* `fg <job>`: Change a stopped or running background job to a running in the foreground.
* `kill <job>`: Terminate a job.

A single command line can be split into multiple jobs. If the command line has multiple commands split with the ampersand "&", the shell will group the commands following the "&" into another job, while running the former job in the background.
~~~bash
csapsh> cp sysprog/sub.zip . & sleep 5 & echo Hello &
~~~
This command line will run three background jobs, each executing `cp`, `sleep`, and `echo` commands.
~~~bash
csapsh> sleep 5 & ls -al
~~~
This command line will run two jobs, one (with `sleep` command) in the background, and the other (with `ls` command) in the foreground.
~~~bash
ps -eo stat,command | grep ./mysplit > file2 & head -n 6 < file > file3 &
~~~
Note that these jobs can have multiple pipes and file I/O redirections.

### The csapsh Specification

Your csapsh shell should have the following features:

* The prompt should be the string `csapsh> `.
* The command line typed by the user should consist of a name and zero or more arguments, all separated by one or more spaces. 
If name is a built-in command, then csapsh should handle it immediately and wait for the next command line. Otherwise, 
csapsh should assume that name is the path of an executable file, which it loads and runs in the context of an initial child process 
(In this context, the term job refers to this initial child process).
* Typing ctrl-c (ctrl-z) should cause a SIGINT (SIGTSTP) signal to be sent to the current foreground job, as well as any descendents of that job
(e.g., any child processes that it forked). If there is no foreground job, then the signal should have no effect.
* If the command line ends with an ampersand `&`, then csapsh should run the job in the background. Otherwise, it should run the job in the foreground.
* If the command line contains multiple `&` characters between commands, then csapsh should add and run multiple jobs separated by `&`s. Note that at most one job (at the end of command line) can be run in the foreground and the others should run in the background.
* Each job can be identified by either its job ID (JID), a positive integer assigned by csapsh, its process group ID, or one of its process IDs (one for simple commands, several for piped command sequences). JIDs should be denoted on the command line by the prefix `%`. Process group IDs use the prefix `@`. For example, "%5" denotes JID 5, "@5" denotes process group 5, and "5" denotes the process with PID 5. We have provided you with all of the routines you need for manipulating the job list.
* csapsh should reap all of its zombie children. If any job terminates because it receives a signal that it didn't catch, then csapsh should recognize this event and print a message with the job's PID and a description of the offending signal.
* csapsh should support the following built-in commands:
  * The `quit` command terminates the shell.
  * The `jobs` command lists all background jobs.
  * The `bg <job>` command restarts `<job>` by sending it a SIGCONT signal, and then runs it in the background. The `<job>` argument can be either be a JID, a process group ID, or a PID.
  * The `fg <job>` command restarts `<job>` by sending it a SIGCONT signal, and then runs it in the foreground. The `<job>` argument can be either be a JID, a process group ID, or a PID.
* csapsh should support input and output file redirection:
  * When executing `csapsh> ls > file`, `ls` should print its output into the specified `file` instead of standard output.  
  * When executing `csapsh> cat < file`, `cat` should get its input from `file`, not from the standard input. Thus it should print the content of `file`.
* csapsh should support I/O redirection between programs via pipes. This means that when executing the following command, `ls` should print its output into a pipe whose read end is connected to standard in of the subsequent `sort` program.  
  `csapsh> ls | sort`  
  Note that more than two programs can be connected via pipes and the output of the last process can be sent to a file:  
  `csapsh> ls | grep "CSAP" | sort > /tmp/result.txt`
* csapsh does not support above features (input & output file redirection and command pipe) for built-in commands. Also, built-in commands does not run in the background.

## Handout Overview

The handout contains the following files and directories

| File/Directory | Description |
|:---  |:--- |
| src/csapsh.c | Skeleton for csapsh.c. Implement your solution by editing this file. |
| src/jobcontrol.c/h | Implementation of job control APIs (add, delete, list, ...). **Do not modify!** |
| src/parser.c/h | Implementation of command line parser. **Do not modify!** |
| reference/ | Reference implementation |
| tools/     | Tools to test your implementation step-by-step |
| traces/    | Traces of command line used to test your implementation |
| Makefile   | Makefile driver program |
| README.md  | this file |


### Tools

The `tools` directory contains example binaries and a test driver file.

| File/Directory | Description |
|:---  |:--- |
| Makefile     | Makefile to build example binaries and run tests. |
| myint.c      | Example binary that prints counter every second and send SIGINT. |
| myprod.c     | Example binary that prints counter every second. |
| myspin.c     | Example binary that prints a string every second. |
| mysplit.c    | Example binary that forks a child process and prints a string every second. |
| mystop.c     | Example binary that prints counter every second and send SIGTSTP. |
| sdriver.pl   | Driver script to test csapsh with traces. |


## Checking Your work

We provide some tools to help you check your work.

### csapsh in verbose mode
The `csapsh` itself has the feature to help your implementation. Running it in verbose mode (with `-v` option) will print helpful informations including the parsed command line arguments, job & signal infos at each functions and signal handlers. If you want to print more informations on what your shell is doing, you can use the `VERBOSE` macro as you need. This will be safer than using naive `printf`s, since mistakenly left `printf`s will affect your test results!

### Reference solution
You can find a reference implementation of csapsh in `reference/csapsh`. Run this program to resolve any questions you have about how your shell should behave. Your shell should emit output that is identical to the reference solution (except for PIDs, of course, which change from run to run).

### Shell driver
The `sdriver.pl` driver located in the `tools/` directory executes a shell as a child process. The driver sends commands and signals to the shell as directed by a trace file, and captures and displays the output from the shell.

Use the `-h` argument to find out the usage of sdriver.pl:
~~~bash
$ ./sdriver.pl -h
Usage: ./sdriver.pl [-hv] -t <trace> -s <shellprog> -a <args>
Options:
  -h            Print this message
  -v            Be more verbose
  -t <trace>    Trace file
  -s <shell>    Shell program to test
  -a <args>     Shell arguments
  -g            Generate output for autograder
~~~

We also provide a number of trace files that you can use in conjunction with the shell driver to test the correctness of your shell. 
The lower-numbered trace files do very simple tests, and the higher-numbered tests do more complicated tests.
~~~bash
$ ./sdriver.pl -t ../traces/trace01.txt -s ../csapsh -a "-p"
~~~
(the `-a "-p"` argument tells your shell not to emit a prompt), or
~~~bash
$ make test01
~~~

Similarly, to compare your result with the reference shell, you can run the trace driver on the reference shell by typing:
~~~bash
$ ./sdriver.pl -t ../traces/trace01.txt -s ../reference/csapsh -a "-p"
~~~
or
~~~bash
$ make rtest01
~~~

The neat thing about the trace files is that they generate the same output you would have gotten had you run your shell interactively (except for an initial comment that identifies the trace). 

Note that the shell does not output process group and process IDs when run with the driver. This is to make it easier for us to check your solution automatically.

## Your Task
Your task is to implement csapsh according to the specification. Your code goes into `csapsh.c` that contains a functional skeleton of a simple Unix shell. 
To help you get started, we have already implemented the less interesting functions. Your assignment is to complete the remaining empty functions listed below. 
As a sanity check for you, we've listed the approximate number of lines of code for each of these functions in our reference solution (which includes lots of comments).
* `eval`: Main routine that parses and interprets the command line. [180 LOC, ~110 excluding comments and empty lines]
* `builtin_cmd`: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [<10 lines]
* `do_bgfg`: Implements the bg and fg built-in commands. [65 lines, ~40 excluding comments and empty lines]
* `waitfg`: Waits for a foreground job to complete. [~10 lines]
* `sigchld_handler`: Catches SIGCHILD signals. [75 lines, ~35 excluding comments and empty lines]
* `sigint_handler`: Catches SIGINT (ctrl-c) signals. [~10 lines]
* `sigtstp_handler`: Catches SIGTSTP (ctrl-z) signals. [~10 lines]

Remember that each time you modify your `csapsh.c` file, you need to type `make` to recompile it. To run your shell, type `csapsh` at the command line:
~~~bash
$ ./csapsh
csapsh> [type commands to your shell here]
~~~

## Hints
* Carefully read Chapter 8 (Exceptional Control Flow) in the textbook.
* Use the trace files to guide the development of your shell. Starting with `trace01.txt`, make sure that your shell produces the identical output as the reference shell. Then move on to trace file `trace02.txt`, and so on.
* The `waitpid`, `kill`, `fork`, `execvp`, `setpgid`, and `sigprocmask` functions will come in very handy. The WUNTRACED and WNOHANG options to waitpid will also be useful.
* When you implement your signal handlers, be sure to send SIGINT and SIGTSTP signals to the entire foreground process group, using "-pid" instead of "pid" in the argument to the kill function. The sdriver.pl program tests for this error.
* One of the tricky parts of this assignment is deciding on the allocation of work between the `waitfg` and `sigchld_handler` functions.  While other solutions are possible, such as calling `waitpid` in both `waitfg` and `sigchld_handler`, we recommend a simple approach that does all the reaping in the handler.
  * In `waitfg`, use a busy loop around the sleep function.
  * In `sigchld_handler`, use exactly one call to `waitpid`.
* In `eval`, the parent must use `sigprocmask` to block SIGCHLD signals before it forks the child, and then unblock these signals, again using `sigprocmask` after it adds the child to the job list by calling `addjob`. Since children inherit the blocked vectors of their parents, the child must be sure to then unblock SIGCHLD signals before it execs the new program.  
The parent needs to block the SIGCHLD signals in this way in order to avoid the race condition where the child is reaped by `sigchld_handler` (and thus removed from the job list) before the parent calls `addjob`.
* First, implement handling the command line with a single job by using the first element of each parsed arrays.
* Programs such as `more`, `less`, `vi`, and `emacs` do strange things with the terminal settings. Don't run these programs from your shell. Stick with simple text-based programs such as `/bin/ls`, `/bin/ps`, and `/bin/echo`. You can check if your shell respects the PATH environment variable by evaluating `trace18.txt`.
* When you run your shell from the standard Unix shell, your shell is running in the foreground process group. If your shell then creates a child process, by default that child will also be a member of the foreground process group. Since typing ctrl-c sends a SIGINT to every process in the foreground group, typing ctrl-c will send a SIGINT to your shell, as well as to every process that your shell created, which obviously isn't correct.  
Here is the workaround: After `fork()`, but before `exec()`, the child process should call `setpgid(0, 0)`, which puts the child in a new process group whose group ID is identical to the child's PID. This ensures that there will be only one process, your shell, in the foreground process group. When you type ctrl-c, the shell should catch the resulting SIGINT and then forward it to the appropriate foreground job (or more precisely, the process group that contains the foreground job).
* Look up dup2's MAN page. It will be helpful to implement output redirection.


<div align="center" style="font-size: 1.75em;">

**Happy coding!**
</div>
