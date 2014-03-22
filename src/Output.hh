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

#ifndef __OUTPUT_HH_DEFINED__
#define __OUTPUT_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Assert.hh"
#include "UCS_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
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

/** the output of the APL interpreter to cout and maybe to a test result file
 **/
class Output
{
public:
   /// the color to be usedd for output
   enum ColorMode
      {
         COLM_UNDEF,    ///< color undefined
         COLM_INPUT,    ///< color for (echo of) input characters (CIN)
         COLM_OUTPUT,   ///< color for normal APL output (COUT)
         COLM_ERROR,    ///< color for debug output (CERR)
         COLM_UERROR,   ///< color for APL error output (UERR)
      };

   /// set the color mode (if colors_enabled). Outputs the escape sequence
   /// for \b mode when the color mode changes
   static void set_color_mode(ColorMode mode);

   /// reset() dout_filebuf
   static void reset_dout();

   /// reset colors to black and white
   static void reset_colors();

   /// set or toggle color mode (implementation of command ]XTERM)
   static void toggle_color(const UCS_string & arg);

   /// initialize curses library
   static void  init();

   /// true if the print semaphore was acquired
   static bool print_sema_held;

   /// escape sequence for CIN colors
   static char color_CIN[21];

   /// escape sequence for COUT colors
   static char color_COUT[21];

   /// escape sequence for CERR colors
   static char color_CERR[21];

   /// escape sequence for UERR colors
   static char color_UERR[21];

   /// escape sequence for resetting colors
   static char color_RESET[21];

   /// foreground color for CIN
   static int color_CIN_foreground;

   /// background color for CIN
   static int color_CIN_background;
 
   /// foreground color for COUT
   static int color_COUT_foreground;

   /// background color for COUT
   static int color_COUT_background;
 
   /// foreground color for CERR
   static int color_CERR_foreground;

   /// background color for CERR
   static int color_CERR_background;
 
   /// foreground color for UERR
   static int color_UERR_foreground;

   /// background color for UERR
   static int color_UERR_background;
 
   /// escape sequences for clear to end of line
   static char clear_EOL[21];

   /// true if curses shall be used for output colors
   static bool use_curses;

protected:
   /// true if colors were changed (and then reset_colors() shall reset
   /// them when leaving the interpreter
   static bool colors_changed;

   /// print one byte on stdout
   static int putc_stderr(int ch);

   /// print one byte on stderr
   static int putc_stdout(int ch);

   /// the current color mode
   static ColorMode color_mode;

   /// true if colors are currently enabled (by XTERM command)
   static bool colors_enabled;
};

#endif // __OUTPUT_HH_DEFINED__
