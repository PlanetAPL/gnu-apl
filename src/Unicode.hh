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

#ifndef __UNICODE_HH_DEFINED__
#define __UNICODE_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
# error This file shall not be #included directly, but by #including Common.hh
#endif

/// One Unicode character
enum Unicode
{
#define char_def(n, u, t, f, p) UNI_ ## n = u,
#define char_df1(n, u, t, f, p) UNI_ ## n = u,
#include "Avec.def"

  Unicode_0       = 0,            ///< End fo unicode string
  Invalid_Unicode = 0x55AA55AA,   ///< An invalid Unicode.
};

#endif // __UNICODE_HH_DEFINED__
