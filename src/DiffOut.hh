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

#ifndef __DIFFOUT_HH_DEFINED__
#define __DIFFOUT_HH_DEFINED__

#include <ostream>
#include <fstream>

#include "Assert.hh"
#include "Simple_string.hh"
#include "UTF8_string.hh"

using namespace std;

/// a filebuf that compares its output with a file.
class DiffOut : public filebuf
{
public:
   /// constructor
   DiffOut(bool _errout)
   : aplout(""),
     errout(_errout)
   { aplout.clear(); }

   /// discard all characters
   void reset()
   { aplout.clear(); }

protected:
   /// overloaded filebuf::overflow()
   virtual int overflow(int c);

   /// return true iff 0-terminated strings apl and ref are different
   bool different(const UTF8 * apl, const UTF8 * ref);

   /// a buffer for one line of APL output
   UTF8_string aplout;

   /// true for error messages, false for normal APL output
   bool errout;
};

#endif // __DIFFOUT_HH_DEFINED__
