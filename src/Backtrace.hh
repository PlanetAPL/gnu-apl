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

#ifndef __BACKTRACE_HH_DEFINED__
#define __BACKTRACE_HH_DEFINED__

#include <vector>
#include <stdint.h>

/// show the current function call stack.
class Backtrace
{
public:
   /// show the current function call stack.
   static void show(const char * file, int line);

protected:
   /// find the source for PC \b pc
   static const char * find_src(int64_t pc);

   /// open file apl.lines
   static void open_lines_file();

   /// show one item in the backtrace
   static void show_item(int idx, char * s);

   /// the status of file apl.lines
   enum _lines_status
      {
        LINES_not_checked,   ///< status is unknown
        LINES_outdated,      ///< file is outdated
        LINES_valid          ///< file is up-to-date
      };

   /// the status of file apl.lines
   static _lines_status lines_status;

   /// a mapping from PCs to source lines.
   struct PC_src
      {
        int64_t pc;             ///< the pc
        const char * src_loc;   ///< the source locstion
      };

   /// a mapping from PCs to source lines.
   static std::vector<PC_src> pc_2_src;
};

#endif // __BACKTRACE_HH_DEFINED__
