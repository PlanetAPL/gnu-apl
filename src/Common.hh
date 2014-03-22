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

#ifndef __COMMON_HH_DEFINED__
#define __COMMON_HH_DEFINED__

#include "../config.h"   // for xxx_WANTED and other macros from ./configure

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#ifndef MAX_RANK_WANTED
#define MAX_RANK_WANTED 6
#endif

enum { MAX_RANK = MAX_RANK_WANTED };

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <complex>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "APL_types.hh"
#include "Assert.hh"
#include "Logging.hh"
#include "SystemLimits.hh"

using namespace std;

extern const char * scriptname;

/// echo of APL input (to stdout)
extern ostream CIN;

/// normal APL output (to stdout)
extern ostream COUT;

/// debug output (to stderr)
extern ostream CERR;

/// debug output (to stderr)
extern ostream UERR;

class UCS_string;

// #define TROUBLESHOOT_NEW_DELETE


void * common_new(size_t size);
void common_delete(void * p);

/// current time as double (seconds since epoch)
APL_time now();

/// Year, Month, Day, hour, minute, second, millisecond
struct YMDhmsu
{
   /// construct YMDhmsu from usec since Jan. 1. 1970 00:00:00
   YMDhmsu(APL_time t);

   /// return usec since Jan. 1. 1970 00:00:00
   APL_time get() const;

   int year;     ///< year, e.g. 2013
   int month;    ///< month 1-12
   int day;      ///< day 1-31
   int hour;     ///< hour 0-23
   int minute;   ///< minute 0-59
   int second;   ///< second 0-59
   int micro;    ///< microseconds 0-999999
};

#ifdef TROUBLESHOOT_NEW_DELETE
inline void * operator new(size_t size)   { return common_new(size); }
inline void   operator delete(void * p)   { common_delete(p); }
#endif

using namespace std;

#define loop(v, e) for (ShapeItem v = 0; v < ShapeItem(e); ++v)

//-----------------------------------------------------------------------------

/// return true iff \b uni is a padding character (used internally).
inline bool is_pad_char(Unicode uni)
   { return ((uni >= UNI_PAD_U2) && (uni <= UNI_PAD_U1)) ||
            ((uni >= UNI_PAD_U0) && (uni <= UNI_PAD_L9)); }

//-----------------------------------------------------------------------------

/// Stringify x.
#define STR(x) #x
/// The current location in the source file.
#define LOC Loc(__FILE__, __LINE__)
/// The location line l in file f.
#define Loc(f, l) f ":" STR(l)

extern ostream & get_CERR();

#define Q(x) get_CERR() << std::left << setw(20) << #x ":" << " '" << x << "' at " LOC << endl;

//-----------------------------------------------------------------------------

/// The total memory that we have.
extern uint64_t total_memory;

#ifdef VALUE_HISTORY_WANTED

   enum { VALUEHISTORY_SIZE = 100000 };
   extern void add_event(const Value * val, VH_event ev, int ia,
                        const char * loc);
#  define ADD_EVENT(val, ev, ia, loc)   add_event(val, ev, ia, loc);

#else

   enum { VALUEHISTORY_SIZE = 0 };
#  define ADD_EVENT(_val, _ev, _ia, _loc)

#endif

/// Function_Line ++ (post increment)
inline int operator ++(Function_Line & fl, int)   { return ((int &)fl)++; }

/// Function_PC ++ (post increment)
inline Function_PC
operator ++(Function_PC & pc, int)
{
const Function_PC ret = pc; 
   pc = Function_PC(pc + 1);
   return ret;
}

/// Function_PC ++ (pre increment)
inline Function_PC &
operator ++(Function_PC & pc)
{
   pc = Function_PC(pc + 1);
   return pc;
}

/// Function_PC -- (pre decrement)
inline Function_PC &
operator --(Function_PC & pc)
{
   pc = Function_PC(pc - 1);
   return pc;
}
//-----------------------------------------------------------------------------

#define uhex  std::hex << uppercase << setfill('0')
#define lhex  std::hex << nouppercase << setfill('0')
#define nohex std::dec << nouppercase << setfill(' ')

/// formatting for hex (and similar) values
#define HEX(x)     "0x" << uhex <<                 int64_t(x) << nohex
#define HEX2(x)    "0x" << uhex << std::right << \
                           setw(2) << int(x) << std::left << nohex
#define HEX4(x)    "0x" << uhex << std::right << \
                           setw(4) << int(x) << std::left << nohex
#define UNI(x)     "U+" << uhex <<          setw(4) << int(x) << nohex

//-----------------------------------------------------------------------------
/// a wrapper for gettext()
inline const char *
_(const char * msgid)
{
#ifdef ENABLE_NLS

   return gettext(msgid);

#else

   return msgid;

#endif
}
//-----------------------------------------------------------------------------

#include "SharedValuePointer.hh"

#endif // __COMMON_HH_DEFINED__
