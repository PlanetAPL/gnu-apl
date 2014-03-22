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

#ifndef __PRINT_BUFFER_HH__DEFINED__
#define __PRINT_BUFFER_HH__DEFINED__

#include "Avec.hh"
#include "Common.hh"
#include "PrintContext.hh"
#include "Shape.hh"
#include "UCS_string.hh"

class Value;
class Cell;

//-----------------------------------------------------------------------------

/// some information about a column to be printed. This info
/// is used to properly align the elements of a column.
class ColInfo
{
public:
   /// properties of an empty column.
   ColInfo()
   : flags(0),
     int_len(0),
     fract_len(0),
     real_len(0),
     imag_len(0)
   {}

   /// update the properties of \b this ColInfo according to
   /// the properties of \b other
   void consider(ColInfo & other);

   /// true if \b this ColInfo has an exponent field
   bool have_expo() const
      { return real_len > (int_len + fract_len); }

   /// some flags
   int flags;

   /// the length of the integral part (of numbers) or total length
   /// I.e. an optional leading sign and the digits before the '.'
   int int_len;

   /// the length of the fraction part (of numbers)
   /// I.e. the leading dot and the ditis until an 'E'
   int fract_len;

   /// the length of the real part (of complex numbers) or total length
   int real_len;

   /// the length of the imaginary part (of complex numbers)
   int imag_len;
};
//-----------------------------------------------------------------------------
/** A two-dimensional Unicode character buffer used for converting APL values
    to their character representations.
 */
class PrintBuffer
{
public:
   /// contructor: empty PrintBuffer
   PrintBuffer();

   /// contructor: a single row PrintBuffer
   PrintBuffer(const UCS_string & ucs, const ColInfo & ci);

   /// contructor: a PrintBuffer from an APL value
   PrintBuffer(const Value & value, const PrintContext & pctx);

   /// return the number of rows
   size_t get_height() const
      { return buffer.size(); }

   /// return the first (and only) line
   UCS_string l1() const
      { Assert (get_height() == 1);   return buffer[0]; }

   /// return line y
   UCS_string get_line(size_t y) const
      { Assert (y < get_height());   return buffer[y]; }
   
   /// return the last line, removing it from this PrintBuffer
   UCS_string remove_last_line();

   /// Return the number of columns.
   uint32_t get_width(uint32_t y) const
      { if (get_height() == 0)   return 0;
        Assert(y < get_height());   return buffer[y].size(); }

   /// Set the char in column x and row y to uc.
   void set_char(uint32_t x, uint32_t y, Unicode uc);

   /// Return the char in column x and row y.
   Unicode get_char(uint32_t x, uint32_t y) const;

   /// prepend buffer with \b count characters \b pad
   void pad_l(Unicode pad, ShapeItem count);

   /// append count charactera \b pad  to buffer
   void pad_r(Unicode pad, ShapeItem count);

   /// append lines to reach height
   void pad_height(Unicode pad, ShapeItem height);

   /// return the characters for style \b pst
   static void get_frame_chars(PrintStyle pst, Unicode & HORI, Unicode & VERT,
                       Unicode & NW, Unicode & NE, Unicode & SE, Unicode & SW);


   /// add a decorator frame around this buffer
   void add_frame(PrintStyle style, uint32_t rank, uint32_t depth);

   /// add an outer frame around this buffer
   void add_outer_frame(PrintStyle style);

   /// print properties of \b this PrintBuffer
   ostream & debug(ostream & out, const char * title = 0) const;

   /// append PrintBuffer \b pb1 to \b this PrintBuffer
   void append_col(const PrintBuffer & pb1);

   /// append UCS_string \b ucs to \b this PrintBuffer
   void append_ucs(const UCS_string & ucs);

   /// append ucs, aligne at char \b align.
   void append_aligned(const UCS_string & ucs, Unicode align);

   /// add padding and a column \b pb to \b this PrintBuffer
   void add_column(Unicode pad, int32_t pad_count, const PrintBuffer & pb);

   /// add row \b pb to \b this PrintBuffer
   void add_row(const PrintBuffer & pb);

   /// return the ColInfo of \b this PrintBuffer
   const ColInfo & get_info() const   { return col_info; }

   /// return the ColInfo of \b this PrintBuffer
   ColInfo & get_info() { return col_info; }

   /// return true iff all strings have the same size.
   bool is_rectangular() const;

   /// return positions for line breaks in wide (i.e.  > ⎕PW) PrintBuffers
   const vector<int> & get_break_points() const
      { return break_points; }

protected:
   /// align this PrintBuffer to col
   void align(ColInfo & col);

   /// align this char-only PrintBuffer to col
   void align_left(ColInfo & col);

   /// align this real-only PrintBuffer to col
   void align_dot(ColInfo & col);

   /// align this complex PrintBuffer to col
   void align_j(ColInfo & col);

   /// pad the fraction part to \b wanted_fract_len with '0'
   void pad_fraction(int wanted_fract_len, bool want_expo);

   /// the character buffer.
   vector<UCS_string> buffer;

   /// points after which a line break (according to ⎕PW) should occur.
   vector<int> break_points;

   /// column properties
   ColInfo col_info;
};

#endif // __PRINT_BUFFER_HH__DEFINED__
