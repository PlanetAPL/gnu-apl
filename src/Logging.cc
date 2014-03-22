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


#include "Logging.hh"

#ifdef DYNAMIC_LOG_WANTED

#define log_def(val, item, txt) bool LOG_ ## item = val;
#include "Logging.def"

void
Log_control(LogId lid, bool ON)
{
   switch(lid)
      {
#define log_def(val, item, txt) case LID_ ## item: LOG_ ## item = ON; return;
#include "Logging.def"
      }
}

bool
Log_status(LogId lid)
{
   switch(lid)
      {
#define log_def(val, item, txt) case LID_ ## item: return LOG_ ## item;
#include "Logging.def"
      }
}

const char *
Log_info(LogId lid)
{
   switch(lid)
      {
#define log_def(val, item, txt) case LID_ ## item: return txt;
#include "Logging.def"
      }

   return 0;
}

#endif

