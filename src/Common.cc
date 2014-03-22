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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "Common.hh"
#include "Output.hh"
#include "Value.hh"

uint64_t total_memory = 0;

const char * scriptname = 0;

//-----------------------------------------------------------------------------
void *
common_new(size_t size)
{
void * ret = malloc(size);
   CERR << "NEW " << HEX(ret) << "-" << HEX((const char *)ret + size)
        << "  (" << HEX(size) << ")" << endl;
   return ret;
}
//-----------------------------------------------------------------------------
void
common_delete(void * p)
{
   CERR << "DEL " << HEX(p) << endl;
   free(p);
}
//-----------------------------------------------------------------------------
APL_time
now()
{
timeval tv_now;
   gettimeofday(&tv_now, 0);

APL_time ret = tv_now.tv_sec;
   ret *= 1000000;
   ret += tv_now.tv_usec;
   return ret;
}
//-----------------------------------------------------------------------------
YMDhmsu::YMDhmsu(APL_time at)
   : micro(at % 1000000)
{
const time_t secs = at/1000000;
tm t;
   gmtime_r(&secs, &t);

   year   = t.tm_year + 1900;
   month  = t.tm_mon  + 1;
   day    = t.tm_mday;
   hour   = t.tm_hour;
   minute = t.tm_min;
   second = t.tm_sec;
}
//-----------------------------------------------------------------------------
APL_time
YMDhmsu::get() const
{
tm t;
   t.tm_year = year - 1900;
   t.tm_mon  = month - 1;
   t.tm_mday = day;
   t.tm_hour = hour;
   t.tm_min  = minute;
   t.tm_sec  = second;

APL_time ret =  mktime(&t);
   ret *= 1000000;
   ret += micro;
   return ret;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Function_PC2 & ft)
{
   return out << ft.low << ":" << ft.high;
}
//-----------------------------------------------------------------------------
ostream &
print_flags (ostream & out, ValueFlags flags)
{
   return out << ((flags & VF_marked)   ?  "M" : "-")
              << ((flags & VF_complete) ?  "C" : "-")
              << ((flags & VF_left)     ?  "&" : "-")
              << ((flags & VF_dirty)    ?  "?" : "-")
              << ((flags & VF_forever)  ?  "∞" : "-");
}
//-----------------------------------------------------------------------------

