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

#include "parser.h"

//--------------------------------------------------------------------------------------------------
// Global variables
//

// ugly references to globals defined in csapsh.c
extern char prompt[];          //   shell prompt
extern int emit_prompt;        //   1: emit prompt; 0: do not emit prompt

extern int verbose; // FOR DEBUG
#define VERBOSE(...)  { if (verbose) { fprintf(stderr, ##__VA_ARGS__); fprintf(stderr, "\n"); } }


//--------------------------------------------------------------------------------------------------
// Commmand line parser functions
//

/// @brief Returns true if char @a c is a regular delimiter (space, tab, '|', or '>') or matches
///        the specific delimiter @a extra.
/// @param c character to examine
/// @param extra specific delimiter
/// @retval true if extra==NONE and @a c is a regular delimiter (' ', '\t', '|', '<', '>')
/// @retval true if extra!=NONE and @a c == @a extra
/// @retval false otherwise
#define NONE '\0'
int isdelim(char c, char extra)
{
  return ((extra == NONE) && ((c == ' ') || (c == '\t') || (c == '|') || (c == '<') || (c == '>')))
         || (c == extra);
}

/// @brief Skip over whitespace in string @a str starting at position @a pos. Returns the position
///        of the first non-whitespace character (or the \0 byte).
/// @param str string to search
/// @param pos starting position
/// @retval int position of first non-whitespace character or the null-byte
int skip_whitespace(const char *str, int pos)
{
  while ((str[pos] == ' ') || (str[pos] == '\t')) pos++;
  return pos;
}

/// @brief Prints an error marker at position @a pos + the length of the prompt and an error
///        message in dependence of @a error. The error codes are purposefully chosen to match
///        the mode in parseline below. Always returns -1.
/// @param cmdline command line containing the error
/// @param pos error position
/// @param error error type
/// @retval -1
int parseline_error(const char *cmdline, int pos, int error)
{
  assert(pos >= 0);

  if (emit_prompt) pos+=(int)strlen(prompt);
  else printf("%s", cmdline);
  printf("%*s^\n", pos, " ");
  switch (error) {
    case 0:  printf("Command expected.\n");
             break;
    case 1:  printf("Argument expected.\n");
             break;
    case 2:  printf("Filename expected.\n");
             break;
    case 3:  printf("Only '>', '<', '&' and '\\n' is excepted.");
             break;
    case 4:  printf("Command or '\\n' is excepted.\n");
             break;
    case 5:  printf("Quoted argument not terminated.\n");
             break;
    case 6:  printf("Out of memory.\n");
             break;
    case 7:  printf("Only one input redirection allowed.\n");
             break;
    case 8:  printf("Only one output redirection allowed.\n");
             break;
    default: printf("Invalid error code: %d\n", error);
  }

  return -1;
}

int parse_cmdline(char *cmdline, JobState **mode, char *****argv, char ***infile, char ***outfile, int **num_cmd, char ***sep_cmds)
{
  assert(cmdline);

  int pos = 0;              // current position in cmdline
  int state = 0;            // 0: argument required
                            // 1: optional arguments
                            // 2: filename required
                            // 3: in/out redirected ('<', '>', '&', and newline allowed) (after filename)
                            // 4: end of background job (first argument of next job and newline allowed) (after '&')
  int inout = 0;            // 0: input redirection
                            // 1: output redirection

  *mode = NULL;
  *infile = NULL;
  *outfile = NULL;
  char ****cmd = NULL;
  int *num_cmds = NULL;
  int job_idx = 0, job_max = 0;   // current & maximum index into job
  int cmd_idx = 0, cmd_max = 0;   // current & maximum index into argv[job_idx]
  int arg_idx = 0, arg_max = 0;   // current & maximum index into argv[job_idx][cmd_idx]

  int job_begin = 0;
  char **cmdlines = NULL;

  while (cmdline[pos] != '\n') {
    // skip whitespace
    pos = skip_whitespace(cmdline, pos);

    switch (cmdline[pos]) {
      case '|':
        { //
          // pipe
          //
          if (state != 1) return parseline_error(cmdline, pos, state);
          pos++;
          cmd_idx++; arg_idx = 0; arg_max = 0;
          state = 0;
          break;
        }

      case '<':
        { //
          // input redirection
          //
          if ((*infile)[job_idx]) return parseline_error(cmdline, pos, 7);
          if ((state != 1) && (state != 3)) return parseline_error(cmdline, pos, state);
          pos++;
          state = 2; inout = 0;
          break;
        }

      case '>':
        { //
          // output redirection
          //
          if ((*outfile)[job_idx]) return parseline_error(cmdline, pos, 8);
          if ((state != 1) && (state != 3)) return parseline_error(cmdline, pos, state);
          pos++;
          state = 2; inout = 1;
          break;
        }

      case '&':
        { //
          // background
          //
          if ((state != 1) && (state != 3)) return parseline_error(cmdline, pos, state);
          num_cmds[job_idx] = cmd_idx + 1;
          cmd_idx = 0, cmd_max = 0;
          arg_idx = 0; arg_max = 0;

          // separate each commands from cmdline while printing
          cmdlines[job_idx] = &cmdline[job_begin];
          cmdline[pos++] = '\0'; // change '&' to '\0' to separate cmdline for each jobs

          (*mode)[job_idx++] = jsBackground;
          state = 4;
          break;
        }

      case '\n':
        { //
          // end of input
          //
          if ((state == 0) || (state == 2)) return parseline_error(cmdline, pos, state);
          break;
        }

      default:
        { //
          // command, argument, or filename
          //
          if (state == 3) return parseline_error(cmdline, pos, state);

          // mark the start pos of the job
          if (cmd_idx == 0 && arg_idx == 0) job_begin = pos;

          // check for quoted arguments
          char extra = NONE;
          if ((cmdline[pos] == '\'') || (cmdline[pos] == '"')) {
            extra = cmdline[pos];
            pos++;
          }

          // find end of argument
          int astart = pos;
          while ((cmdline[pos] != '\n') && (!isdelim(cmdline[pos], extra))) pos++;
          int aend = pos;

          if (extra != NONE) {
            if (cmdline[pos] == extra) pos++;                // include closing quote
            else return parseline_error(cmdline, astart, 5); // no matching end quote found
          }

          // extract argument
          char *argument = malloc(aend-astart+1);
          strncpy(argument, &cmdline[astart], aend-astart);
          argument[aend-astart] = '\0';

          if (state != 2) {
            // job/command/argument

            if (job_idx >= job_max-1) {
              job_max += 8;

              cmd = realloc(cmd, job_max * sizeof(cmd[0]));
              if (cmd == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=job_idx; i<job_max; i++) cmd[i] = NULL;

              *mode = realloc(*mode, job_max * sizeof((*mode)[0]));
              if (*mode == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=job_idx; i<job_max; i++) (*mode)[i] = jsUndefined;

              *infile = realloc(*infile, job_max * sizeof((*infile)[0]));
              if (*infile == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=job_idx; i<job_max; i++) (*infile)[i] = NULL;

              *outfile = realloc(*outfile, job_max * sizeof((*outfile)[0]));
              if (*outfile == NULL) return parseline_error(cmdline, pos, 6);
              for(int i=job_idx; i<job_max; i++) (*outfile)[i] = NULL;

              num_cmds = realloc(num_cmds, job_max * sizeof(num_cmds[0]));
              if (num_cmds == NULL) return parseline_error(cmdline, pos, 6);
              for(int i=job_idx; i<job_max; i++) num_cmds[i] = 0;

              cmdlines = realloc(cmdlines, job_max * sizeof(cmdlines[0]));
              if (cmdlines == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=job_idx; i<job_max; i++) cmdlines[i] = NULL;
            }

            if (cmd_idx >= cmd_max-1) {
              // resize argv[job_idx]
              cmd_max += 8;
              cmd[job_idx] = realloc(cmd[job_idx], cmd_max * sizeof(cmd[0][0]));
              if (cmd[job_idx] == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=cmd_idx; i<cmd_max; i++) cmd[job_idx][i] = NULL;
            }
            if (arg_idx >= arg_max-1) {
              // resize argv[job_idx][cmd_idx]
              arg_max += 8;
              cmd[job_idx][cmd_idx] = realloc(cmd[job_idx][cmd_idx], arg_max * sizeof(cmd[0][0][0]));
              if (cmd[job_idx][cmd_idx] == NULL) return parseline_error(cmdline, pos, 6);
              for (int i=arg_idx; i<arg_max; i++) cmd[job_idx][cmd_idx][i] = NULL;
            }

            cmd[job_idx][cmd_idx][arg_idx++] = argument;
            if((*mode)[job_idx] == jsUndefined) (*mode)[job_idx] = jsForeground;

            state = 1;
          } else {
            // filename
            if (inout == 0) (*infile)[job_idx] = argument;
            else (*outfile)[job_idx] = argument;
            state = 3;
          }
        }
    }
  }
  if ((cmd != NULL) && ((state == 0) || (state == 2))) return parseline_error(cmdline, pos, state);

  // update foreground job's num_cmd value
  // loop exit with state value of 1 or 3 denotes the lastest job is running foreground
  if ((state == 1) || (state == 3)) {
    num_cmds[job_idx] = cmd_idx + 1;
    cmdlines[job_idx] = &cmdline[job_begin];
    cmdline[pos]='\0';
  }

  *argv = cmd;
  *num_cmd = num_cmds;
  *sep_cmds = cmdlines;

  if ((state == 0) && (job_idx == 0)) return 0;
  else if (state == 4) return job_idx;
  else return job_idx + 1;
}


void dump_cmdstruct(char ***cmd, char *infile, char *outfile, JobState mode)
{
  if (cmd == NULL) return;

  size_t cmd_idx = 0, arg_idx;

  while (cmd[cmd_idx] != NULL) {
    printf("    argv[%lu]:\n", cmd_idx);
    arg_idx = 0;
    while (cmd[cmd_idx][arg_idx] != NULL) {
      printf("      argv[%lu][%lu] = %s\n", cmd_idx, arg_idx, cmd[cmd_idx][arg_idx]);
      arg_idx++;
    }
    cmd_idx++;
  }

  if (infile) printf("Input redirection from %s.\n", infile);
  if (outfile) printf("Output redirection to %s.\n", outfile);
  printf("Command runs in %sground.\n", mode == jsForeground ? "fore" : "back");
}


void free_cmdstruct(char ****cmd, char **infile, char **outfile, JobState *mode)
{
  if (cmd == NULL) return;

  int job_idx = 0, cmd_idx, arg_idx;

  while (cmd[job_idx] != NULL) {
    cmd_idx = 0;
    while (cmd[job_idx][cmd_idx] != NULL) {
      arg_idx = 0;
      while (cmd[job_idx][cmd_idx][arg_idx] != NULL) free(cmd[job_idx][cmd_idx][arg_idx++]);
      free(cmd[job_idx][cmd_idx++]);
    }
    if (infile[job_idx]) free(infile[job_idx]); 
    if (outfile[job_idx]) free(outfile[job_idx]);
    free(cmd[job_idx++]);
  }
  if (mode) free(mode);
}

