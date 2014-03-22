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

#ifndef __UTF8_STRING_HH_DEFINED__
#define __UTF8_STRING_HH_DEFINED__

#include <iostream>
#include <stdint.h>
#include <string.h>

#include "Simple_string.hh"

using namespace std;

class UCS_string;

//-----------------------------------------------------------------------------
/// one byte of a UTF8 encoded Unicode (RFC 3629) string
typedef uint8_t UTF8;

//-----------------------------------------------------------------------------
/// an UTF8 encoded Unicode (RFC 3629) string
class UTF8_string :  public Simple_string<UTF8>
{
public:
   /// constructor: empty UTF8_string
   UTF8_string()
   : Simple_string<UTF8>((const UTF8 *)0, 0)
   {}

   /// constructor: UTF8_string from 0-terminated C string. The C-string
   /// must contain only ASCII characters (i.e. is NOT UTF8 encoded) !
   UTF8_string(const char * string)
   : Simple_string<UTF8>((const UTF8 *)string, strlen(string))
   {}

   /// constructor: copy of string, but at most len bytes
   UTF8_string(const UTF8 * string, size_t len)
   : Simple_string<UTF8>(string, len)
   {}

   /// constructor: copy of UCS string. The UCS characters will be UTF8-encoded
   UTF8_string(const UCS_string & ucs);

   /// constructor: UCS_string from (simple character vector) APL value.
   /// Non-ASCII The UCS characters will be UTF8 encoded.
   UTF8_string(const Value & value);

   const char * c_str()
      {
        extend(items_valid + 1);
        items[items_valid] = 0;   // the termnating 0
        return (const char *)items;
      }

   /// convert the first char in UTF8-encoded string to Unicode, set
   /// setting len to the number of bytes in the UTF8 encoding of the char
   static Unicode toUni(const UTF8 * string, int & len);

   /// return the next UTF8 encoded char from an input file
   static Unicode getc(istream & in);
};
//-----------------------------------------------------------------------------

#endif // __UTF8_STRING_HH_DEFINED__
