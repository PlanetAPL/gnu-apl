/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Common.hh"   // for HAVE_EXECINFO_H
#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
# include <cxxabi.h>
#define EXEC(x) x
#else
#define EXEC(x)
#endif

#include "Backtrace.hh"

using namespace std;

Backtrace::_lines_status 
       Backtrace::lines_status = Backtrace::LINES_not_checked;

vector<Backtrace::PC_src> Backtrace::pc_2_src;

//-----------------------------------------------------------------------------
const char *
Backtrace::find_src(int64_t pc)
{
int pos = 0;
int len = pc_2_src.size();

   if (len == 0)          return 0;   // database not set up.
   if (pc == -1LL)        return 0;   // no pc

   while (len > 1)
      {
        const int low_len = len/2;
        const int middle = pos + low_len;
        const int64_t middle_pc = pc_2_src[middle].pc;

        if      (pc < middle_pc)   {               len = low_len;  }
        else if (pc > middle_pc)   { pos = middle; len -= low_len; }
        else                       { pos = middle;   break;        }
      }

   return pc_2_src[pos].src_loc;
}
//-----------------------------------------------------------------------------
void
Backtrace::open_lines_file()
{
   if (lines_status != LINES_not_checked)   return;

   // we are here for the first time.
   // Assume that apl2.lines is outdated until proven otherwise.
   //
   lines_status = LINES_outdated;

struct stat st;
   if (stat("apl2", &st))   return;   // stat apl2 failed
 
const time_t apl2_time = st.st_mtime;

const int fd = open("apl2.lines", O_RDONLY);
   if (fd == -1)   return;

   if (fstat(fd, &st))   // stat apl2.lines failed
      {
        close(fd);
        return;
      }

const time_t apl2_lines_time = st.st_mtime;

   if (apl2_lines_time < apl2_time)
      {
        close(fd);
        cerr << endl
             << "Cannot show line numbers, since apl2.lines is older "
                "than apl2." << endl
             << "Run 'make apl2.lines' to fix this" << endl;
        return;
      }

   pc_2_src.reserve(100000);
char buffer[1000];
FILE * file = fdopen(fd, "r");
   assert(file);

const char * src_line = 0;
bool new_line = false;
int64_t prev_pc = -1LL;

   for (;;)
       {
         const char * s = fgets(buffer, sizeof(buffer) - 1, file);
         if (s == 0)   break;   // end of file.

         buffer[sizeof(buffer) - 1] = 0;
         const int len = strlen(buffer);
         if (buffer[len - 1] == '\n')   buffer[len - 1] = 0;

         // we look for 2 types of line: abolute source file paths
         // and code lines.
         //
         if (s[0] == '/')   // source file path
            {
              src_line = strdup(strrchr(s, '/') + 1);
              new_line = true;
              continue;
            }

         if (!new_line)   continue;

         if (s[8] == ':')   // opcode address
            {
              int64_t pc = -1LL;
              if (1 == sscanf(s, " %llx:", &pc))
                 {
                   if (pc < prev_pc)
                      {
                         assert(0 && "apl2.lines not ordered by PC");
                         break;
                      }

                   PC_src pcs = { pc, src_line };
                   pc_2_src.push_back(pcs);
                   prev_pc = pc;
                   new_line = false;
                 }
              continue;
            }
       }

   fclose(file);   // also closes fd

   cerr << "read " << pc_2_src.size() << " line numbers" << endl;
   lines_status = LINES_valid;
}
//-----------------------------------------------------------------------------
void
Backtrace::show_item(int idx, char * s)
{
#ifdef HAVE_EXECINFO_H
  //
  // s looks like this:
  //
  // ./apl(_ZN10APL_parser9nextTokenEv+0x1dc) [0x80778dc]
  //

   // split off address from s.
   //
const char * addr = "????????";
int64_t pc = -1LL;
   {
     char * space = strchr(s, ' ');
     if (space)
        {
          *space = 0;
          addr = space + 2;
          char * e = strchr(space + 2, ']');
          if (e)   *e = 0;
          pc = strtoll(addr, 0, 16);
        }
   }

const char * src_loc = find_src(pc);

   // split off function from s.
   //
char * fun = 0;
   {
    char * opar = strchr(s, '(');
    if (opar)
       {
         *opar = 0;
         fun = opar + 1;
          char * e = strchr(opar + 1, ')');
          if (e)   *e = 0;
       }
   }

   // split off offset from fun.
   //
int offs = 0;
   if (fun)
      {
       char * plus = strchr(fun, '+');
       if (plus)
          {
            *plus++ = 0;
            sscanf(plus, "%X", &offs);
          }
      }

char obuf[200] = "@@@@";
   if (fun)
      {
        strncpy(obuf, fun, sizeof(obuf) - 1);
        obuf[sizeof(obuf) - 1] = 0;

       size_t obuflen = sizeof(obuf) - 1;
       int status = 0;
//     cerr << "mangled fun is: " << fun << endl;
       __cxxabiv1::__cxa_demangle(fun, obuf, &obuflen, &status);
       switch(status)
          {
            case 0: // demangling succeeded
                 break;

            case -2: // not a valid name under the C++ ABI mangling rules.
                 break;

            default:
                 cerr << "__cxa_demangle() returned " << status << endl;
                 break;
          }
      }

// cerr << setw(2) << idx << ": ";

   cerr << (const void *)pc;
// cerr << left << setw(20) << s << right << " ";

   // indent.
   for (int i = -1; i < idx; ++i)   cerr << " ";

   cerr << obuf;

// if (offs)   cerr << " +" << offs;

   if (src_loc)   cerr << " at " << src_loc;
   cerr << endl;
#endif
}
//-----------------------------------------------------------------------------
void
Backtrace::show(const char * file, int line)
{
   // CYGWIN, for example, has no execinfo.h and the functions declared there
   //
#ifndef HAVE_EXECINFO_H
   CERR << "Cannot show function call stack since execinfo.h seems not"
           " to exist on this OS (WINDOWs ?)." << endl;
   return;
#else


   open_lines_file();

void * buffer[200];
const int size = backtrace(buffer, sizeof(buffer)/sizeof(*buffer));

char ** strings = backtrace_symbols(buffer, size);

   cerr << endl
        << "----------------------------------------"  << endl
        << "-- Stack trace at " << file << ":" << line << endl
        << "----------------------------------------"  << endl;
   for (int i = 1; i < size - 1; ++i)
       {
         // make a copy of strings[i] that can be
         // messed up in show_item().
         //
         const char * si = strings[size - i - 1];
         char cc[strlen(si) + 1];
         strcpy(cc, si);
         cc[sizeof(cc) - 1] = 0;
         show_item(i - 1, cc);
       }
   cerr << "========================================" << endl;

#if 0
   // crashes at times
   free(strings);   // but not strings[x] !
#endif


#endif
}
//-----------------------------------------------------------------------------
