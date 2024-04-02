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
/// CONTRIBUTORS  BE LIABLE FOR ANY DIRECT,  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSE-
/// QUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF
/// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
/// DAMAGE.
//--------------------------------------------------------------------------------------------------

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jobcontrol.h"

//--------------------------------------------------------------------------------------------------
// Global variables
//

static Job *jobs = NULL;       ///< job list

// ugly references to globals defined in main shell driver
extern int emit_prompt;        //   1: emit prompt; 0: do not emit prompt


//--------------------------------------------------------------------------------------------------
// Job list manipulation functions
//


/// @brief Return the highest ID of all allocated jobs
/// @retval int job ID
static int getmaxid(void)
{
  Job *j = jobs;
  int id = 0;

  while (j != NULL) {
    if (j->jid > id) id = j->jid;
    j = j->next;
  }

  return id;
}


int addjob(pid_t pgid, pid_t *pid, int nproc, JobState state, char *cmdline)
{
  assert(pgid > 0);
  assert((nproc > 0) && pid);

  // allocate memory for new job
  Job *j = calloc(1, sizeof(Job));
  assert(j != NULL);

  // initialize job
  j->jid = getmaxid()+1;
  j->pgid = pgid;
  j->pid = pid;
  j->nproc_tot = nproc;
  j->nproc_cur = nproc;
  j->state = state;
  memcpy(j->cmdline, cmdline, strlen(cmdline));;
  j->cmdline[MAXLINE-1] = '\0';

  // debug
  //print_job(j);

  // insert into list
  j->next = jobs;
  jobs = j;

  return j->jid;
}


int deletejob(int jid)
{
  // located job in list
  Job *p = NULL, *j = jobs;
  while ((j != NULL) && (j->jid != jid)) { p = j; j = j->next; }

  // remove & free if found
  if (j != NULL) {
    if (p == NULL) jobs = j->next;
    else p->next = j->next;

    if (j->pid != NULL) free(j->pid);
    free(j);
  }

  // success if we found & deleted the job
  return j != NULL;
}


Job* getjob_jid(int jid)
{
  Job *j = jobs;

  while ((j != NULL) && (j->jid != jid)) j = j->next;

  return j;
}


Job* getjob_pgid(pid_t pgid)
{
  Job *j = jobs;

  while ((j != NULL) && (j->pgid != pgid)) j = j->next;

  return j;
}


Job* getjob_pid(pid_t pid)
{
  Job *j = jobs;

  while (j != NULL) {
    for (int p=0; p<j->nproc_tot; p++) {
      if (j->pid[p] == pid) return j;
    }

    j = j->next;
  }

  return NULL;
}


Job* getjob_foreground(void)
{
  Job *j = jobs;

  while ((j != NULL) && (j->state != jsForeground)) j = j->next;

  return j;
}

/// @brief print a job (internal use only)
/// @param job job to print
static void print_job(Job *job)
{
  assert(job);
  int showpids = emit_prompt == 1;

  printf("[%d] (%d) ", job->jid, showpids ? job->pgid : -1);

  printf("{ ");
  for (int p=0; p<job->nproc_tot; p++) printf("%d ", showpids ? job->pid[p] : -1);
  printf("} ");

  switch (job->state) {
    case jsBackground: printf("Running ");    break;
    case jsForeground: printf("Foreground "); break;
    case jsStopped:    printf("Stopped ");    break;
    case jsUndefined:  printf("Undefined");   break;
    default: printf("listjobs: Internal error: state=%d ", job->state);
  }

  printf("%s\n", job->cmdline);
}


void printjob(int jid)
{
  Job *job = getjob_jid(jid);

  if (job != NULL) print_job(job);
  else printf("Job %d not found.\n", jid);
}


void listjobs(void)
{
  if (jobs != NULL) {
    Job *j = jobs;
    while (j != NULL) {
      print_job(j);
      j = j->next;
    }
  } else {
    printf("No jobs.\n");
  }
}

