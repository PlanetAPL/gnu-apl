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

#ifndef __LINE_INPUT_HH_DEFINED__
#define __LINE_INPUT_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Assert.hh"
#include "UCS_string.hh"

using namespace std;

/*
 o The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (cin)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) |
     +---+---+             +---------+---------+
         |                           |
         |                           |
         V                           |
      +--+--+                        |
      | APL |                        |
      +--+--+                        |
         |                           |
         |                           |
         +-----------------------+   |
         |                       |   |
         |                       V   V
         |                    +--+---+--+
         |                    | compare |
         |                    +----+----+
         |                         |
         |                         |
         V                         V
       (cout)                (test results)
 
 */

/** the input for the APL interpreter from either the user (via readline)
    or from testcase files.
 */
class Input
{
public:
   /// return the readline version used, 0 if readline was disabled
   static int readline_version();

   /// get one line with trailing blanks removed
   static UCS_string get_line();

   /// get one line from \b filename with trailing blanks removed
   static UTF8 * get_f_line(FILE * filename);

   /// print prompt and return one line from the user
   static UCS_string get_quad_cr_line(UCS_string prompt);

   /// Exit the readline library, editing the history file.
   static void exit_readline();

   /// like get_user_line, but making the prompt editable
   /// (and part of the user input)
   static const char * get_user_line_nabla(const UCS_string * prompt);

   /// the name of a file from the -f file command line option
   static const char * input_file_name;

   /// a FILE * for an opened \b input_file (0 if none)
   static FILE * input_file_FILE;

   /// initialize readline library
   static void init();

   /// true if readline lib is present and shall be used
   static bool use_readline;

protected:
   /// Read one line from the user.
   static const unsigned char * get_user_line(const UCS_string * prompt,
                                              bool from_nabla);

private:
   /// true iff -f - was given
   static bool stdin_from_file;
};

#endif // __LINE_INPUT_HH_DEFINED__
