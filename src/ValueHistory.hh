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

#ifndef __VALUEHISTORY_HH_DEFINED__
#define __VALUEHISTORY_HH_DEFINED__

#include "Common.hh"
#include <ostream>

using namespace std;

class Value;

class VH_entry
{
   friend void print_history(ostream & out, const Value * val,
                             const char * loc);

public:
   VH_entry() {}

   VH_entry(const Value * val, VH_event ev, int ia, const char * loc);

   static void init();

   static VH_entry history[VALUEHISTORY_SIZE];
   static int idx;

protected:
   void print(int & flags, ostream & out, const Value * val,
               const VH_entry * previous) const;

   const Value * val;
   VH_event      event;
   int           iarg;
   const char  * loc;
   const char  * testcase_file;
   int           testcase_line;
};

extern void print_history(ostream & out, const Value * val,
                                const char * loc);
#endif // __VALUEHISTORY_HH_DEFINED__
