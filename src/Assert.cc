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

#include <string.h>

#include "Assert.hh"
#include "Backtrace.hh"
#include "Common.hh"
#include "Output.hh"
#include "TestFiles.hh"
#include "Workspace.hh"

/// prevent recursive do_Assert() calls.
static bool asserting = false;

//-----------------------------------------------------------------------------
void
do_Assert(const char * cond, const char * fun, const char * file, int line)
{
const int loc_len = strlen(file) + 40;
char * loc = new char[loc_len + 1];

   Log(LOG_delete)
      CERR << "new    " << (const void *)loc << " at " LOC << endl;

   snprintf(loc, loc_len, "%s:%d", file, line);
   loc[loc_len] = 0;

   CERR << endl
        << "======================================="
           "=======================================" << endl;


   if (cond)       // normal assert()
      {
        CERR << "Assertion failed: " << cond << endl
             << "in Function:      " << fun  << endl
             << "in file:          " << loc  << endl << endl;
      }
   else if (fun)   // segfault etc.
      {
        CERR << "\n\n================ " << fun <<  " ================\n";
      }

   CERR << "Call stack:" << endl;

   if (asserting)
      {
        CERR << "*** do_Assert() called recursively ***" << endl;
      }
   else
      {
        asserting = true;

        Backtrace::show(file, line);

        CERR << endl << "SI stack:" << endl << endl;
        Workspace::list_SI(CERR, SIM_SIS_dbg);
      }
   CERR << "======================================="
           "=======================================" << endl;

   // count errors
   TestFiles::assert_error();

Error * err = Workspace::get_error();
   if (err)   err->init(E_ASSERTION_FAILED, loc);

   asserting = false;

   throw E_ASSERTION_FAILED;
}
//-----------------------------------------------------------------------------

