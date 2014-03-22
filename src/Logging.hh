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

#ifndef __LOGGING_HH_DEFINED__
#define __LOGGING_HH_DEFINED__

#include "Common.hh"

enum LogId
{
   LID_NONE = 0,
#define log_def(val, item, txt) LID_ ## item,
#include "Logging.def"
   LID_MAX,
   LID_MIN = LID_NONE + 1,
};

#ifdef DYNAMIC_LOG_WANTED

#define log_def(val, item, txt) extern bool LOG_ ## item;
#include "Logging.def"

void Log_control(LogId lid, bool ON);
bool Log_status(LogId lid);
const char * Log_info(LogId lid);

#else   // static logging

#define Log_control(x, y)
#define log_def(val, item, txt) enum { LOG_ ## item = val };
#include "Logging.def"

#endif

/// Shortcut for the selected logging option.
#define Log(x) if (x)

#endif // __LOGGING_HH_DEFINED__

