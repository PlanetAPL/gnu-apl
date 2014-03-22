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
#include <stdio.h>

#include <vector>

#include "Avec.hh"
#include "Common.hh"
#include "Error.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

/* from RFC 3629 / STD 63:

   Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
   --------------------+---------------------------------------------
   0000 0000-0000 007F | 0xxxxxxx
   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

   RFC 2279, the predecessor of RFC 3629, also allowed these:

   0000 0000-0000 007F   0xxxxxxx
   0000 0080-0000 07FF   110xxxxx 10xxxxxx
   0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
   0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
   0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx

   In order to be more general, we follow RFC 2279 and allow 5-byte and 6-byte
   encodings
*/

//-----------------------------------------------------------------------------
UTF8_string::UTF8_string(const UCS_string & ucs)
   : Simple_string<UTF8>((const UTF8 *)0, 0)
{
   Log(LOG_char_conversion)
      CERR << "UTF8_string::UTF8_string(ucs = " << ucs << ")" << endl;

   loop(i, ucs.size())
      {
        uint32_t uni = ucs[i];
        if (uni < 0x80)            // 1-byte unicode (ASCII)
           {
             append(uni);
           }
        else if (uni < 0x800)      // 2-byte unicode
           {
             const uint8_t b1 = uni & 0x3F;   uni >>= 6; 
             append(uni | 0xC0);
             append(b1  | 0x80);
           }
        else if (uni < 0x10000)    // 3-byte unicode
           {
             const uint8_t b2 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b1 = uni & 0x3F;   uni >>= 6; 
             append(uni | 0xE0);
             append(b1  | 0x80);
             append(b2  | 0x80);
           }
        else if (uni < 0x200000)   // 4-byte unicode
           {
             const uint8_t b3 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b2 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b1 = uni & 0x3F;   uni >>= 6; 
             append(uni | 0xF0);
             append(b1  | 0x80);
             append(b2  | 0x80);
             append(b3  | 0x80);
           }
        else if (uni < 0x4000000)   // 5-byte unicode
           {
             const uint8_t b4 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b3 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b2 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b1 = uni & 0x3F;   uni >>= 6; 
             append(uni | 0xF8);
             append(b1  | 0x80);
             append(b2  | 0x80);
             append(b3  | 0x80);
             append(b4  | 0x80);
           }
        else if (uni < 0x80000000)   // 6-byte unicode
           {
             const uint8_t b5 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b4 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b3 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b2 = uni & 0x3F;   uni >>= 6; 
             const uint8_t b1 = uni & 0x3F;   uni >>= 6; 
             append(uni | 0xFC);
             append(b1  | 0x80);
             append(b2  | 0x80);
             append(b3  | 0x80);
             append(b4  | 0x80);
             append(b5  | 0x80);
           }
        else
           {
             CERR << "Bad Unicode: " << UNI(uni) << endl;
             Backtrace::show(__FILE__, __LINE__);
             Assert(0 && "Error in UTF8_string::UTF8_string(ucs)");
           }
      }

   Log(LOG_char_conversion)
      CERR << "UTF8_string::UTF8_string(): utf = " << *this << endl;
}
//-----------------------------------------------------------------------------
UTF8_string::UTF8_string(const Value & value)
{
UCS_string ucs(value);
   new (this) UTF8_string(ucs);
}
//-----------------------------------------------------------------------------
Unicode
UTF8_string::toUni(const UTF8 * string, int & len)
{
const uint32_t b0 = *string++;
   if (b0 < 0x80)                  { len = 1;   return Unicode(b0); }

uint32_t bx = b0;   // the "significant" bits in b0
   if ((b0 & 0xE0) == 0xC0)        { len = 2;   bx &= 0x1F; }
   else if ((b0 & 0xF0) == 0xE0)   { len = 3;   bx &= 0x0F; }
   else if ((b0 & 0xF8) == 0xF0)   { len = 4;   bx &= 0x0E; }
   else if ((b0 & 0xFC) == 0xF8)   { len = 5;   bx &= 0x0E; }
   else if ((b0 & 0xFE) == 0xFC)   { len = 6;   bx &= 0x0E; }
   else
      {
        CERR << "Bad UTF8 sequence: " << HEX(b0);
        loop(j, 6)
           {
             const uint32_t bx = string[j];
             if (bx & 0x80)   CERR << " " << HEX(bx);
             else              break;
           }

        CERR <<  " at " LOC << endl;

        Backtrace::show(__FILE__, __LINE__);
        Assert(0 && "Internal error in UTF8_string::toUni()");
      }

uint32_t uni = 0;
   loop(l, len - 1)
       {
         const UTF8 subc = *string++;
         if ((subc & 0xC0) != 0x80)
            {
              CERR << "Bad UTF8 sequence: " << HEX(b0) << "... at " LOC << endl;
              Assert(0 && "Internal error in UTF8_string::toUni()");
            }

         bx  <<= 6;
         uni <<= 6;
         uni |= subc & 0x3F;
       }

   return Unicode(bx | uni);
}
//-----------------------------------------------------------------------------
Unicode
UTF8_string::getc(istream & in)
{
const uint32_t b0 = in.get() & 0xFF;
   if (!in.good())         return Invalid_Unicode;
   if      (b0 < 0x80)   { return Unicode(b0);    }

uint32_t bx = b0;   // the "significant" bits in b0
int len;

   if      ((b0 & 0xE0) == 0xC0)   { len = 1;   bx &= 0x1F; }
   else if ((b0 & 0xF0) == 0xE0)   { len = 2;   bx &= 0x0F; }
   else if ((b0 & 0xF8) == 0xF0)   { len = 3;   bx &= 0x0E; }
   else
      {
        CERR << "Bad UTF8 sequence: " << HEX(b0) << "... at " LOC << endl;
        Backtrace::show(__FILE__, __LINE__);
        return Invalid_Unicode;
      }

char cc[4];
   in.get(cc, len + 1);   // read subsequent characters + terminating 0
   if (!in.good())   return Invalid_Unicode;

uint32_t uni = 0;
   loop(l, len)
       {
         const UTF8 subc = cc[l];
         if ((subc & 0xC0) != 0x80)
            {
              CERR << "Bad UTF8 sequence: " << HEX(b0) << "... at " LOC << endl;
              return Invalid_Unicode;
            }

         bx  <<= 6;
         uni <<= 6;
         uni |= subc & 0x3F;
       }

   return Unicode(bx | uni);
}
//-----------------------------------------------------------------------------
ostream &
operator<<(ostream & os, const UTF8_string & utf)
{
   loop(c, utf.size())   os << utf[c];
   return os;
}
//-----------------------------------------------------------------------------
