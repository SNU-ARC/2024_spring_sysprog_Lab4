//--------------------------------------------------------------------------------------------------
// Shell Lab                               Spring 2024                           System Programming
//
/// @file
/// @brief command line parser
/// @author CS:APP & CSAP lab
/// @section changelog Change Log
/// 2021/11/14 Bernhard Egger refactored to support proper job control (try "sleep 3 | ls")
/// 2021/11/20 Bernhard Egger add support for input redirection
/// 2024/05/11 ARC lab add support for multiple job handling
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

#ifndef __PARSE_H__
#define __PARSE_H__

#include "jobcontrol.h"

/// @brief parses a command line and splits it into separate jobs which consists of separate argv
///        arrays. The separate argv arrays can be used directly with execv. The first to thrid
///        level arrays are NULL terminated. The ampersand character ('&') separates jobs, and the
///        pipe character ('|') separates commands. The filename following the I/O input 
///        redirection character ('<') is returnd in its job index of @a infile. The filename
///        following the I/O output redirection character ('>') is returned in its job index of 
///        @a outfile. Number of piped commands for each job is returned in its job index of 
///        @a num_cmd. String pointers to the commands of each jobs are returned in its job index
///        of @a sep_cmds. Returns the number of individual jobs found in the command line or -1
///        on error.
///
/// The syntax of a command line is as follows:
///   cmdline = job { "&" job } ["&"].
///   job     = command { "|" command } [ inredir ] [ outredir ]
///   command = cmd { arg }.
///   inredir = "<" filename
///   outredir= ">" filename
///
/// That is, a command line consists of at least one job, followed by 0 or more background job
/// commands. Job command consists of at least one commands follwed by 0 or more piped commands,
/// and optional I/O redirection.
///
/// A command consists of the name of the executable followed by 0 or more arguments. Commands
/// and arguments enclosed in quotes are treated a a single entity.
///
/// Examples:
///   csapsh> ls -l /tmp
///
///   csapsh> ls -l /tmp | sort -r
///
///   csapsh> ls -l /tmp | wc > statistics.txt
///
///   csapsh> sort -r < statistics.txt
///
///   csapsh> sleep 10 &
///
///   csapsh> ls -l /tmp | sort | shuf > listing.txt &
///
///   csapsh> ls -l /tmp | sort > listing.txt & sort -r < statistics.txt
///
///         >> Result:
///         >> *mode = { jsBackground, jsForeground }
///         >> *argv = { { { "ls", "-l", "/tmp" }, { "sort" } }, { { "sort ", "-r" } } }
///                      | +------ cmd 0 -------+  +- cmd1 -+ |  | +--- cmd 0 ---+ |
///                      +---------------- job 0 -------------+  +----- job 1 -----+
///         >> *infile  = { NULL, "statistics.txt" }
///         >> *outfile = { "listing.txt", NULL }
///         >> *num_cmd = { 2, 1 }
///         >> sep_cmds = { { &cmdline[0], &cmdline[13] }, { &cmdline[34] } }
///         >> return: 2
///
///
/// @param cmdline command line to parse
/// @param[out] mode NULL-terminated array of job state (jsForeground or jsBackground, undefined
///                  on error)
/// @param[out] argv NULL-terminated two dimenssional array of 'char *argv[]' arrays suitable for
///                  the execv family
/// @param[out] infile array of NULL or name of file to redirect stdin of (first ) command to
/// @param[out] outfile array of NULL or name of file to redirect stdout of (last) command to
/// @param[out] num_cmd array of number of commands for each job
/// @param[out] sep_cmds array of character pointers for each job
/// @retval >=0 number of distinct jobs in @a cmdline
/// @retval -1 on error
int parse_cmdline(char *cmdline, JobState **mode, char *****argv , char ***infile, char ***outfile, int **num_cmd, char ***sep_cmds);


/// @brief prints a single parsed command structure to stdout (Note that it prints a SINGLE
///        command for each call)
///
/// @param cmd array of char *argv[] arrays obtained by calling @a parseline.
/// @param infile filename for input redirection or NULL
/// @param outfile filename for output redirection or NULL
/// @param mode execution mode
void dump_cmdstruct(char ***cmd, char *infile, char *outfile, JobState mode);


/// @brief frees a parsed command structure
///
/// @param cmd array of char **argv[] arrays obtained by calling @a parseline
/// @param infile array of filenames for input redirection or NULL
/// @param outfile array of filenames for output redirection or NULL
/// @param mode array of execution modes
void free_cmdstruct(char ****cmd, char **infile, char **outfile, JobState *mode);

#endif // __PARSE_H__
