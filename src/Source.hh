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

#ifndef __SOURCE_HH_DEFINED__
#define __SOURCE_HH_DEFINED__

#include "Simple_string.hh"

//-----------------------------------------------------------------------------
/// A substring of a string for sequential access
template<typename T>
class Source
{
public:
   /// constructor: source is entire string.
   Source(const Simple_string<T> & s)
   : str(s),
   idx(0),
   end(s.size())
   {}

   /// constructor: source is string from \b from to the end of the string
   Source(const Source<T> & src, int32_t from)
   : str(src.str),
     idx(src.idx + from),
     end(src.end)
   {
     if (idx > end)   idx = end;
   }

   /// constructor: source is string from \b from to \b to
   Source(const Source<T> & src, int32_t from, int32_t to)
   : str(src.str),
     idx(src.idx + from),
     end(src.idx + from + to)
   {
     if (end > src.str.size())   end = src.str.size();
     if (idx > end)   idx = end;
   }

   /// return the number of remaining items.
   int32_t rest() const
      { return end - idx; }

   /// lookup next item
   const T & operator[](int32_t i) const
      { i += idx;   Assert(uint32_t(i) < uint32_t(end));   return str[i]; }

   /// get next item
   const T & get()
      { Assert(idx < end);   return str[idx++]; }

   /// get last item
   const T & get_last()
      { Assert(idx < end);   return str[--end]; }

   /// get the end (excluding)
   int32_t get_end() const
      { return end; }

   /// lookup next item without removing it
   const T & operator *() const
      { Assert(idx < end);   return str[idx]; }

   /// skip the first element
   void operator ++()
      { Assert(idx < end);   ++idx; }

   /// undo skip of the first element
   void operator --()
      { Assert(idx > 0);   --idx; }

   /// shrink the source to rest \b new_rest
   void set_rest(int32_t new_rest)
      { Assert(new_rest <= rest());   end = idx + new_rest; }

   /// skip \b count elements
   void skip(int32_t count)
      { idx += count;   if (idx > end)   idx = end; }

   /// return the number of \b qs (e.g. spaces) before (excluding)
   /// the current item
   int32_t count_before(const T & q) const
      {
        int32_t p = idx - 1;
        for (; p  >= 0; --p)   if (str[p] != q)   break;
        return idx - p - 1;
      }

protected:
   /// the source string
   const Simple_string<T> & str;

   /// the current position
   int32_t idx;

   /// the end position (excluding)
   int32_t end;
};

#endif // __SOURCE_HH_DEFINED__
