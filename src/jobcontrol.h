//--------------------------------------------------------------------------------------------------
// Shell Lab                               Spring 2024                           System Programming
//
/// @file
/// @brief job control
/// @author CS:APP & CSAP lab
/// @section changelog Change Log
/// 2021/11/14 Bernhard Egger refactored to support proper job control (try "sleep 3 | ls")
/// 2024/05/11 ARC lab improved for 2024 class
///
/// @section license_section License
/// Copyright CS:APP authors
/// Copyright (c) 2020-2023, Computer Systems and Platforms Laboratory, SNU
/// Copyright (c) 2024, Architecture and Code Optimization Laboratory, SNU
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without modification, are permitted
/// provided that the following conditions are met:
///
/// - Redistributions of source code must retain the above copyright notice, this list of condi-
///   tions and the following disclaimer.
/// - Redistributions in binary form must reproduce the above copyright notice, this list of condi-
///   tions and the following disclaimer in the documentation and/or other materials provided with
///   the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
/// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY
/// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
/// CONTRIBUTORS BE  LIABLE FOR ANY DIRECT,  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSE-
/// QUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF
/// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
/// DAMAGE.
//--------------------------------------------------------------------------------------------------

#ifndef __JOBCONTROL_H__
#define __JOBCONTROL_H__

#include <unistd.h>

//--------------------------------------------------------------------------------------------------
// Limits and constant definitions
//
#define MAXLINE    1024      ///< max. length of command line

//--------------------------------------------------------------------------------------------------
// Data types
//

/// @brief jobs states
///
/// The job state transitions and enabling actions are as follows
///   foreground -> stopped    : ctrl-z or SIGSTOP
///   stopped    -> foreground : fg command
///   stopped    -> background : bg command
///   foreground -> undefined  : ctrl-c or signal terminating job
/// At most one job can be in the foreground at any given time.
typedef enum _jobstate {
  jsUndefined,               ///< undefined
  jsForeground,              ///< running in the foreground
  jsBackground,              ///< running in the background
  jsStopped,                 ///< stopped
} JobState;


/// @brief Job struct containing information about a job
typedef struct job {
  struct job *next;          ///< next job
  int jid;                   ///< job ID
  pid_t pgid;                ///< group ID of process group. All processes must have the same GID
  pid_t *pid;                ///< PIDs of all shell-invoked processes of this job
  int nproc_tot;             ///< total (initial) number of processes in this job
  int nproc_cur;             ///< current number of processes in this job
  JobState state;            ///< job state
  char cmdline[MAXLINE];     ///< command line
} Job;


//--------------------------------------------------------------------------------------------------
// Job list manipulation functions
//

/// @brief Add a job to the job list and return its id.
///
/// @param pgid process group ID
/// @param pid array of process IDs
/// @param nproc number of processes in pid array (total/inital number of processes)
/// @param state job state
/// @param cmdline command line
/// @retval int job ID of added process
/// @retval -1 on failure
int addjob(pid_t pgid, pid_t *pid, int nproc, JobState state, char *cmdline);

/// @brief Delete job with job ID @a jid from job list
/// @param jid job ID
/// @retval 1 on success
/// @retval 0 on failure
int deletejob(int jid);

/// @brief find a job by its ID
/// @param jid job ID
/// @retval Job* job
/// @retval NULL if no such job exists
Job* getjob_jid(int jid);

/// @brief find a job by its process group ID
/// @param pgid process group ID
/// @retval Job* job
/// @retval NULL if no such job exists
Job* getjob_pgid(pid_t pgid);

/// @brief find a job by one of its process IDs
/// @param pid process ID
/// @retval Job* job
/// @retval NULL if no such job exists
Job* getjob_pid(pid_t pid);

/// @brief find and return the current foreground job
/// @retval job_t* pointer to job struct
/// @retval NULL if no such job is in the foreground
Job* getjob_foreground(void);

/// @brief print a job with a given job id @a jid
/// @param jid job id
void printjob(int jid);

/// @brief print job list
void listjobs(void);

#endif // __JOBCONTROL_H__
