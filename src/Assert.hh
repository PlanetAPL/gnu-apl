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

#ifndef __ASSERT_HH_DEFINED__
#define __ASSERT_HH_DEFINED__

#include "Common.hh"

extern void do_Assert(const char * cond, const char * fun,
                      const char * file, int line)
#ifdef __GNUC__
    __attribute__ ((noreturn))
#endif
;

#ifndef ASSERT_LEVEL_WANTED

#error "ASSERT_LEVEL_WANTED not defined"

#elif ASSERT_LEVEL_WANTED == 0

#define Assert1(x)
#define Assert(x)

#elif ASSERT_LEVEL_WANTED == 1

#define Assert1(x)
#define Assert(x)  if (!(x))   do_Assert(#x, __FUNCTION__, __FILE__, __LINE__)

#elif ASSERT_LEVEL_WANTED == 2

#define Assert1(x) if (!(x))   do_Assert(#x, __FUNCTION__, __FILE__, __LINE__)
#define Assert(x)  if (!(x))   do_Assert(#x, __FUNCTION__, __FILE__, __LINE__)

#else 

#error "Bad ASSERT_LEVEL_WANTED"

#endif

#define NeverReach(X) do_Assert(X, __FUNCTION__, __FILE__, __LINE__)

/// trivial assertion

/// normal assertion

/// assertion being fatal if wrong
#define Assert_fatal(x) if (!(x)) {\
   cerr << endl << endl << "FATAL error at " << __FILE__ << ":" << __LINE__ \
        << endl;   exit(2); }

#endif // __ASSERT_HH_DEFINED__

