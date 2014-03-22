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

#include <stdio.h>
#include <string.h>

#include "Function.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintBuffer.hh"
#include "PrintOperator.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer()
{
}
//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer(const UCS_string & ucs, const ColInfo & ci)
   : col_info(ci)
{
   buffer.push_back(ucs);
}
//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer(const Value & value, const PrintContext & _pctx)
{
const PrintStyle outer_style = _pctx.get_style();
const bool framed = outer_style & (PST_CS_MASK | PST_CS_OUTER);
PrintContext pctx(_pctx);
   pctx.set_style(PrintStyle(outer_style &~ PST_CS_OUTER));

   if (value.is_skalar())
      {
        PrintContext pctx1(pctx);
        if (value.get_ravel(0).need_scaling(pctx))   pctx1.set_scaled();

        *this = value.get_ravel(0).character_representation(pctx1);

        // pad the value unless it is framed
        if (value.compute_depth() > 1 && !framed)
           {
                pad_l(UNI_PAD_U4, 1);
                pad_r(UNI_PAD_U5, 1);
           }

        add_outer_frame(outer_style);
        return;
      }

const ShapeItem ec = value.element_count();
   if (ec == 0)   // empty value of any dimension
      {
         const Shape sh = value.get_shape().without_axis(sh.get_rank() - 1);
         if (sh.get_volume() <= 1)   // empty vector
            {
              if (pctx.get_style() == PR_APL_FUN)
                 {
                   if (value.get_ravel(0).is_character_cell())   // ''
                      {
                        UCS_string ucs;
                        ucs.append(UNI_SINGLE_QUOTE);
                        ucs.append(UNI_SINGLE_QUOTE);

                        ColInfo ci;
                        *this = PrintBuffer(ucs, ci);
                        add_outer_frame(outer_style);
                        return;
                      }
                 }
              add_outer_frame(outer_style);
              return;   // 0 rows
            }

         // value has > 0 rows. Compute how many lines we need.
         //
         ShapeItem lines = sh.get_volume();
         loop(s, sh.get_rank())
             lines += s * (sh.get_shape_item(sh.get_rank() - s - 1));

         buffer.resize(lines);
         add_outer_frame(outer_style);
         return;
      }

const ShapeItem cols = value.get_last_shape_item();
const ShapeItem rows = ec/cols;

   if (pctx.get_style() == PR_APL_FUN)
      {
        UCS_string ucs;

        if (value.is_char_vector())
           {
             ucs.append(UNI_SINGLE_QUOTE);
             loop(e, ec)
                {
                  const Unicode uni = value.get_ravel(e).get_char_value();
                  ucs.append(uni);
                  if (uni == UNI_SINGLE_QUOTE)   ucs.append(uni);   // ' -> ''
                }
             ucs.append(UNI_SINGLE_QUOTE);
           }
        else
           {
             loop(e, ec)
                {
                  PrintBuffer pb = value.get_ravel(e)
                                        .character_representation(pctx);
                  if (e)   ucs.append(UNI_ASCII_SPACE);
                  ucs.append(UCS_string(pb, 0));
                }
           }

        ColInfo ci;
        *this = PrintBuffer(ucs, ci);
        add_outer_frame(outer_style);
        return;
      }
   else if (pctx.get_style() & PST_HEXDUMP)
      {
        UCS_string ucs;
        const char * char_fmt;
        const char * int_fmt;
        if (pctx.get_style() & PST_HEXlower)
           {
             if (pctx.get_style() & PST_PACKED)   char_fmt = "%04x";
             else                                 char_fmt = "%02x";
             if (sizeof(APL_Integer) == 4)        int_fmt  = "%08x";
             else                                 int_fmt  = "%016x";
           }
        else
           {
             if (pctx.get_style() & PST_PACKED)   char_fmt = "%04X";
             else                                 char_fmt = "%02X";
             if (sizeof(APL_Integer) == 4)        int_fmt  = "%08X";
             else                                 int_fmt  = "%016X";
           }

        loop(e, ec)
           {
             const Cell & cell = value.get_ravel(e);
             char cc[200];
             if (cell.is_character_cell())
                {
                  const Unicode uni = cell.get_char_value();
                  snprintf(cc, sizeof(cc) - 1, char_fmt, uni);
                }
              else if (cell.is_integer_cell())
                {
                  const APL_Integer val = cell.get_int_value();
                  snprintf(cc, sizeof(cc) - 1, int_fmt, val);
                  
                }
              else
                {
                  CERR << "??celltype= " << cell.get_classname()
                       << " ??" << endl;;
                  DOMAIN_ERROR;
                }

              loop(c, sizeof(cc))
                 {
                   if (cc[c])   ucs.append(Unicode(cc[c]));
                   else         break;
                 }
           }

        ColInfo ci;
        *this = PrintBuffer(ucs, ci);
        add_outer_frame(outer_style);
        return;
      }

   // 1. create a vector with a bool per column that tells if the
   // column needs scaling (i.e. exponential format) or not
   //
vector<bool> scaling;
   {
     scaling.reserve(cols);
     loop(x, cols)
        {
          bool need_scaling = false;
          loop(y, rows)
             {
               if (value.get_ravel(x + y*cols).need_scaling(pctx))
                  {
                    need_scaling = true;
                    break;
                  }
             }
          scaling.push_back(need_scaling);
        }
   }

   // 2. create a matrix of items.
   //    An item of the matrix is a top-level value (possibly nested),
   //    therefore we have (⍴,value) items. Items are rectangular.
   //
typedef vector<PrintBuffer> PB_row;
vector<PB_row> item_matrix;

   item_matrix.reserve(rows);
   loop(y, rows)
      {
        vector<PrintBuffer> item_row;
        item_row.reserve(cols);
        ShapeItem row_height = 0;

        loop(x, cols)
            {
              PrintContext pctx1 = pctx;
              if (scaling[x])   pctx1.set_scaled();
              const Cell & cell = value.get_ravel(x + y*cols);
              PrintBuffer item = cell.character_representation(pctx1);

              if (row_height < item.get_height())
                 row_height = item.get_height();

              Assert(item.is_rectangular());
              item_row.push_back(item);
            }

        // pad all items to the same height
        loop(x, cols)
           {
             item_row[x].pad_height(UNI_PAD_U6, row_height);
             Assert(item_row[x].is_rectangular());
           }
        item_matrix.push_back(item_row);
      }


   // 3. align all columns (which pads them to the same width).
   //
   loop(x, cols)
      {
        ColInfo col_info_x;
        loop(y, rows)   col_info_x.consider(item_matrix[y][x].get_info());

        loop(y, rows)   item_matrix[y][x].align(col_info_x);

      }

   // 4. combine colums, then rows.
   //
int32_t last_spacing = 0;
bool last_notchar = false;
int break_width = 0;
   loop(x, cols)
      {
        PrintBuffer pcol;

        loop(y, rows)
         {
           // separator row(s)
           if (y)
              {
                ShapeItem prod = 1;
                loop(r, value.get_rank() - 2)
                   {
                     prod *= value.get_shape_item(value.get_rank() - r - 2);
                     if (y % prod == 0)
                        pcol.append_ucs(UCS_string(pcol.get_width(0),
                                                      UNI_PAD_L0));
                     else
                        break;
                   }
              }

             const PB_row & row = item_matrix[y];
             const PrintBuffer & item = row[x];
             if (y)   pcol.add_row(item);
             else     pcol = item;
         }

        bool not_char = false;   // determined by get_col_spacing()
	const int32_t col_spacing = value.get_col_spacing(not_char, x, framed);

        const int32_t max_spacing = (col_spacing > last_spacing) 
                                  ?  col_spacing : last_spacing;
        int32_t no_char_spacing = 0;

        if (x == 0)
           {
              *this = pcol;
              break_width = get_width(0);
           }
        else
           {
              if (last_notchar)
                 {
                   pad_r(UNI_PAD_U2, 1);
                   ++no_char_spacing;
                 }
              else if (not_char)
                 {
                   pcol.pad_l(UNI_PAD_U3, 1);
                   ++no_char_spacing;
                 }

              // compute ⎕PW breakpoints.
              {
              const int col_width = max_spacing + pcol.get_width(0);
              if ((break_width + col_width) > pctx.get_PW())   // exceed ⎕PW
                 {
                   break_points.push_back(break_width + max_spacing);
                   break_width = pcol.get_width(0);
                 }
              else
                 {
                   break_width += col_width;
                 }
              }

              // append the column. we want a total spacing of 'max_spacing'
              // but we deduct the 'no_char_spacing' chars ² and ³
              // that were appended already.
              //
              add_column(UNI_PAD_U7, max_spacing - no_char_spacing, pcol);
           }

        last_spacing = col_spacing;
        last_notchar = not_char;
      }

   if (value.compute_depth() > 1 && !framed)
      {
        pad_l(UNI_PAD_U8, 1);
        pad_r(UNI_PAD_U9, 1);
      }

   if (!is_rectangular())
      {
        Q(get_height())
        loop(h, get_height())   CERR << "w=" << get_width(h) << "*" << endl;
        loop(h, get_height())   CERR << "*" << get_line(h) << "*" << endl;
      }

   Assert(is_rectangular());
   add_outer_frame(outer_style);

// debug("PrintBuffer::PrintBuffer(Value_P, ...)");
}
//-----------------------------------------------------------------------------
UCS_string
PrintBuffer::remove_last_line()
{
   Assert(get_height() > 0);

UCS_string ucs = buffer[get_height() - 1];
   buffer.resize(get_height() - 1);

   return ucs;
}

//-----------------------------------------------------------------------------
Unicode
PrintBuffer::get_char(uint32_t x, uint32_t y) const
{ 
   Assert(y < buffer.size());
   Assert(x < buffer[y].size());
   return buffer[y][x];
}
//-----------------------------------------------------------------------------
void
PrintBuffer::set_char(uint32_t x, uint32_t y, Unicode uc)
{
   Assert(y < buffer.size());
   Assert(x < buffer[y].size());
   buffer[y][x] = uc;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_l(Unicode pad, ShapeItem count)
{
   loop(y, get_height())   buffer[y].insert(0, count, pad);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_r(Unicode pad, ShapeItem count)
{
UCS_string ucs(count, pad);
   loop(y, get_height())   buffer[y].append(ucs);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_height(Unicode pad, ShapeItem height)
{
   if (height > get_height())
      {
        UCS_string ucs(get_width(0), pad);
        while (height > get_height())   buffer.push_back(ucs);
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::get_frame_chars(PrintStyle pst,
                             Unicode & HORI, Unicode & VERT,
                             Unicode & NW, Unicode & NE,
                             Unicode & SE, Unicode & SW)
{
   switch(pst & PST_CS_MASK)
      {
        case PST_CS_ASCII:
             HORI = UNI_ASCII_MINUS;
             VERT = UNI_ASCII_BAR;
             NW   = UNI_ASCII_FULLSTOP;
             NE   = UNI_ASCII_FULLSTOP;
             SE   = UNI_SINGLE_QUOTE;
             SW   = UNI_SINGLE_QUOTE;
             break;

        case PST_CS_THIN:
             HORI = UNI_LINE_HORI;
             VERT = UNI_LINE_VERT;
             NW   = UNI_LINE_DOWN_LEFT;
             NE   = UNI_LINE_DOWN_RIGHT;
             SE   = UNI_LINE_UP_RIGHT;
             SW   = UNI_LINE_UP_LEFT;
             break;

        case PST_CS_THICK:
             HORI = UNI_LINE_HORI2;
             VERT = UNI_LINE_VERT2;
             NW   = UNI_LINE_DOWN2_LEFT2;
             NE   = UNI_LINE_DOWN2_RIGHT2;
             SE   = UNI_LINE_UP2_RIGHT2;
             SW   = UNI_LINE_UP2_LEFT2;
             break;

        case PST_CS_DOUBLE:
             HORI = UNI_LINE2_HORI;
             VERT = UNI_LINE2_VERT;
             NW   = UNI_LINE2_DOWN_LEFT;
             NE   = UNI_LINE2_DOWN_RIGHT;
             SE   = UNI_LINE2_UP_RIGHT;
             SW   = UNI_LINE2_UP_LEFT;
             break;

        default:
             HORI = Unicode_0;
             VERT = Unicode_0;
             NW   = Unicode_0;
             NE   = Unicode_0;
             SE   = Unicode_0;
             SW   = Unicode_0;
             FIXME;
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_frame(PrintStyle style, uint32_t rank, uint32_t depth)
{
   Assert(is_rectangular());

Unicode HORI, VERT, NW, NE, SE, SW;
   get_frame_chars(style, HORI, VERT, NW, NE, SE, SW);

   if (get_height() == 0)   // empty
      {
        UCS_string upper;
        upper.append(NE);
        upper.append(NW);
        buffer.push_back(upper);

        UCS_string lower;
        lower.append(SE);
        lower.append(SW);
        buffer.push_back(lower);

        Assert(is_rectangular());
        return;
      }

   // draw a bar left and right
   //
   loop(y, get_height())
      {
        buffer[y].insert(0, 1, VERT);
        buffer[y].append(VERT);
      }

   // draw a bar on top and bottom.
   //
UCS_string hori(get_width(0), HORI);

   buffer.insert(buffer.begin(), hori);
   buffer.push_back(hori);

   // draw the corners
   //
   set_char(0,                0,                NE);
   set_char(0,                get_height() - 1, SE);
   set_char(get_width(0) - 1, 0,                NW);
   set_char(get_width(0) - 1, get_height() - 1, SW);

   // draw other decorators
   //
   if (rank > 0)   set_char(1, 0, UNI_RIGHT_ARROW);
   if (rank > 1)   set_char(0, 1, UNI_DOWN_ARROW);
   if (depth)
      {
        loop(d, depth - 1)
            if (d + 1 < get_width(0) - 1)
               set_char(1 + d, get_height() - 1, UNI_ELEMENT1);
      }

   if (style & PST_EMPTY_LAST)   // last (X-) dimension is empty
      {
        set_char(1, 0, UNI_CIRCLE_BAR);   // ⊖
      }

   if (style & PST_EMPTY_NLAST)   // a non-last (Y-) dimension is empty
      {
        set_char(0, 1, UNI_CIRCLE_STILE);   // ⌽
      }

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_outer_frame(PrintStyle style)
{
   style = PrintStyle(style >> 4 & PST_CS_MASK);
   if (style == PST_CS_NONE)   return;

Unicode HORI, VERT, NW, NE, SE, SW;
   get_frame_chars(style, HORI, VERT, NW, NE, SE, SW);

   if (get_height() == 0)   // empty
      {
        UCS_string upper;
        upper.append(NE);
        upper.append(NW);
        buffer.push_back(upper);

        UCS_string lower;
        lower.append(SE);
        lower.append(SW);
        buffer.push_back(lower);

        Assert(is_rectangular());
        return;
      }

   // draw a bar left and right
   //
   loop(y, get_height())
      {
        buffer[y].insert(0, 1, VERT);
        buffer[y].append(VERT);
      }

   // draw a bar on top and bottom.
   //
UCS_string hori(get_width(0), HORI);

   buffer.insert(buffer.begin(), hori);
   buffer.push_back(hori);

   // draw the corners
   //
   set_char(0,                0,                NE);
   set_char(0,                get_height() - 1, SE);
   set_char(get_width(0) - 1, 0,                NW);
   set_char(get_width(0) - 1, get_height() - 1, SW);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
ostream &
PrintBuffer::debug(ostream & out, const char * title) const
{
   if (title)   out << title << endl;

   if (get_height() == 0)
      {
        out << UNI_LINE_DOWN_RIGHT << UNI_LINE_DOWN_LEFT << endl
            << UNI_LINE_UP_RIGHT   << UNI_LINE_UP_LEFT
            << "  flags=" << HEX(col_info.flags)
            << "  len="  << col_info.int_len
            << "."   << col_info.fract_len
            << endl << endl;
        return out;
      }

   out << UNI_LINE_DOWN_RIGHT;
   loop(w, get_width(0))   out << UNI_LINE_HORI;
   out << UNI_LINE_DOWN_LEFT << endl;

   loop(y, get_height())
   out << UNI_LINE_VERT << buffer[y] << UNI_LINE_VERT << endl;

   out << UNI_LINE_UP_RIGHT;
   loop(w, get_width(0))   out << UNI_LINE_HORI;
   out << UNI_LINE_UP_LEFT
       << "  flags=" << HEX(col_info.flags)
       << "  ilen="  << col_info.int_len
       << "  flen="   << col_info.fract_len
       << "  rlen="   << col_info.real_len
       << endl << endl;

   return out;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_col(const PrintBuffer & pb1)
{
   Assert(get_height() == pb1.get_height());

   loop(h, get_height())   buffer[h].append(pb1.buffer[h]);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_ucs(const UCS_string & ucs)
{
   if (buffer.size() == 0)   // empty
      {
        buffer.push_back(ucs);
      }
   else if (ucs.size() < get_width(0))
      {
        UCS_string ucs1(ucs);
        UCS_string pad(get_width(0) - ucs.size(), UNI_PAD_L1);
        ucs1.append(pad);
        buffer.push_back(ucs1);
      }
   else if (ucs.size() > get_width(0))
      {
        UCS_string pad(ucs.size() - get_width(0), UNI_PAD_L2);
        loop(h, get_height())   buffer[h].append(pad);
        buffer.push_back(ucs);
      }
   else
      {
        buffer.push_back(ucs);
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_aligned(const UCS_string & ucs, Unicode align)
{
   Assert(is_rectangular());

int ucs_pos = -1;
   loop(u, ucs.size())
      {
        if (ucs[u] == align)
           {
             ucs_pos = u;
             break;
           }
      }

int this_pos = -1;
   loop(y, buffer.size())
      {
        const UCS_string & row = buffer[y];
        loop(u, row.size())
          {
            if (row[u] == align)
               {
                 this_pos = u;
                 break;
               }
          }
        if (this_pos != -1)   break;
      }

const int ucs_w = ucs.size();
const int this_w = get_width(0);

int ucs_l = 0;    // padding left of ucs
int ucs_r = 0;    // padding right of ucs
int this_l = 0;   // padding left of this
int this_r = 0;   // padding right of this

   if (ucs_pos == -1)       // no align char in ucs
      if (this_pos == -1)   // no align char in this pb
         {
           // no align char at all: align at right border
           //
           //      TTTTttt
           //      UUUUuuu
           //
           if (this_w > ucs_w)   ucs_l = this_w - ucs_w;
           else                  this_l = ucs_w - this_w;
         }
      else                  // align char only in this pb
         {
           //
           //      TTTTttt.ttt
           //      UUUUuuu
           //
           ucs_r = this_w - this_pos;
           if (this_pos > ucs_w)   ucs_l = this_pos - ucs_w;
           else                    this_l = ucs_w - this_pos;
         }
   else                     // align char in ucs
      if (this_pos == -1)   // no align char in this pb
         {
           //
           //      TTTTttt
           //      UUUUuuu.uuu
           //
           this_r = ucs_w - ucs_pos;
           if (ucs_pos > this_w)   this_l = ucs_pos - this_w;
           else                    ucs_l = this_w - ucs_pos;
         }
      else                  // align char in this pb
         {
           //
           //      TTTTttt.tttTTTT
           //      UUUUuuu.uuuUUUU
           //
           if (ucs_pos > this_pos)   this_l = ucs_pos - this_pos;
           else                      ucs_l = this_pos - ucs_pos;
           const int uu = ucs_w - ucs_pos;
           const int tt = this_w - this_pos;
           if (uu > tt)   this_r = uu - tt;
           else           ucs_r = tt - uu;
         }

   Assert(ucs_l >= 0);
   Assert(ucs_r >= 0);
   Assert(this_l >= 0);
   Assert(this_r >= 0);

UCS_string ucs1;

   if (ucs_l > 0)   ucs1.append(UCS_string(ucs_l, UNI_ASCII_SPACE));
   ucs1.append(ucs);
   if (ucs_r > 0)   ucs1.append(UCS_string(ucs_r, UNI_ASCII_SPACE));

   if (this_l > 0)   pad_l(UNI_ASCII_SPACE, this_l);
   if (this_r > 0)   pad_r(UNI_ASCII_SPACE, this_r);

   buffer.push_back(ucs1);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_column(Unicode pad, int32_t pad_count, const PrintBuffer & pb)
{
   if (get_height() != pb.get_height())
      {
         debug(CERR, "this");
         pb.debug(CERR, "pb");
      }

   Assert(get_height() == pb.get_height());

   if (pad_count)
      {
        UCS_string ucs(pad_count, pad);
        loop(y, get_height())   buffer[y].append(ucs);
      }

   loop(y, get_height())   buffer[y].append(pb.buffer[y]);
}
//-----------------------------------------------------------------------------
void PrintBuffer::add_row(const PrintBuffer & pb)
{
   buffer.reserve(buffer.size() + pb.get_height());
   loop(h, pb.get_height())   buffer.push_back(pb.buffer[h]);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align(ColInfo & cols)
{
   // this PrintBuffer is a (possibly nested) APL value.
   // Align the buffer:
   //
   // to the J (in a column containing complex numbers), or
   // to the decimal point (in a column containing non-complex numbers), or
   // to the left (in a column containing text or nested values).
   //
   // make sure that the item is (and remains) rectangular).
   //
   Assert(is_rectangular());

   if      (cols.flags & has_j)          align_j(cols);
   else if ((cols.flags & CT_NUMERIC))   align_dot(cols);
   else                                  align_left(cols);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_dot(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_dot(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   Assert(buffer.size() > 0);

   Assert(COL_INFO.int_len   >= col_info.int_len);
   Assert(COL_INFO.fract_len >= col_info.fract_len);
   Assert(COL_INFO.real_len  >= col_info.real_len);

   if (col_info.flags & CT_NUMERIC)
      {
        // dot-align numeric items
        if (COL_INFO.int_len > col_info.int_len)
           {
             const size_t diff = COL_INFO.int_len - col_info.int_len;
             pad_l(UNI_PAD_L3, diff);
             col_info.real_len += diff;
             col_info.int_len = COL_INFO.int_len;
           }

        if (COL_INFO.fract_len > col_info.fract_len)
           {
             pad_fraction(COL_INFO.fract_len, COL_INFO.have_expo());
           }

        if (COL_INFO.real_len > col_info.real_len)
           {
             const size_t diff = COL_INFO.real_len - col_info.real_len;
             if (col_info.have_expo())   // alrady have an expo field
                {
                  pad_r(UNI_PAD_L4, diff);
                }
             else                        // no expo yet: create one
                {
                  Assert1(diff >= 2);
                  pad_r(UNI_ASCII_E, 1);
                  pad_r(UNI_ASCII_0, 1);
                  pad_r(UNI_PAD_L4, diff - 2);
                }
             col_info.real_len = COL_INFO.real_len;
           }
      }
   else
      {
        // right-align char items
        size_t LEN = COL_INFO.int_len + COL_INFO.fract_len;
        size_t len = col_info.int_len + col_info.fract_len;
        if (LEN > len)
           {
             const size_t diff = LEN - len;
             pad_l(UNI_PAD_L5, diff);
             col_info.int_len   = COL_INFO.int_len;
             col_info.fract_len = COL_INFO.fract_len;
           }
      }

   Log(LOG_printbuf_align)   debug(CERR, "after align_dot()");
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_j(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_j(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   Assert(buffer.size() > 0);

   Assert(COL_INFO.real_len >= col_info.real_len);
   Assert(COL_INFO.imag_len >= col_info.imag_len);

   if (col_info.flags & CT_NUMERIC)
      {
        // J-align numeric items
        if (COL_INFO.real_len > col_info.real_len)
           {
             const size_t diff = COL_INFO.real_len - col_info.real_len;
             pad_l(UNI_PAD_L3, diff);
             col_info.real_len = COL_INFO.real_len;
           }

        if (COL_INFO.imag_len > col_info.imag_len)
           {
             const size_t diff = COL_INFO.imag_len - col_info.imag_len;
             pad_r(UNI_PAD_L4, diff);
             col_info.imag_len = COL_INFO.imag_len;
           }
      }
   else
      {
        // right-align char items
        size_t LEN = COL_INFO.real_len + COL_INFO.imag_len;
        size_t len = col_info.real_len + col_info.imag_len;
        if (LEN > len)
           {
             const size_t diff = LEN - len;
             pad_l(UNI_PAD_L5, diff);
             col_info.real_len = COL_INFO.real_len;
             col_info.imag_len = COL_INFO.imag_len;
           }
      }

   Log(LOG_printbuf_align)   debug(CERR, "after align_j()");
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_fraction(int wanted_fract_len, bool want_expo)
{
const int diff = wanted_fract_len - col_info.fract_len;
   Assert1(diff > 0);

      // copy integer part of this PrintBuffer to to new_buf
      //
UCS_string new_buf(buffer[0], 0, col_info.int_len);

      // copy fractional part to new_buf. If the number has no exponent part,
      // then we fill with spaces. Otherwise fill with '0', possibly inserting
      // a decimal point.
      //
      loop(f, col_info.fract_len)   new_buf.append(buffer[0][col_info.int_len + f]);
      if (!want_expo)
         {
           loop(d, diff)   new_buf.append(UNI_PAD_L4);
         }
      else if (col_info.fract_len == 0)   // no fractional part yet
         {
           new_buf.append(UNI_ASCII_FULLSTOP);
           loop(d, diff - 1)   new_buf.append(UNI_ASCII_0);
         }
      else
         {
           loop(d, diff)   new_buf.append(UNI_ASCII_0);
         }

   // copy exponent part
   //
   for (int ex = col_info.int_len + col_info.fract_len;
        ex < buffer[0].size(); ++ex)
       new_buf.append(buffer[0][ex]);

   col_info.fract_len = wanted_fract_len;
   col_info.real_len += diff;

   buffer[0] = new_buf;

   // if buffer is multi-len then pad remaining line to new length
   //
   for (ShapeItem h = 1; h < get_height(); ++h)
      {
        const int diff = new_buf.size() - get_width(h);
        if (diff > 0)
           {
             const UCS_string ucs(diff, UNI_PAD_L4);
             buffer[h].append(ucs);
           }
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_left(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_left(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   if (col_info.int_len == COL_INFO.int_len)   return;   // no padding needed.

   Assert(col_info.int_len < COL_INFO.int_len);

const size_t diff = COL_INFO.int_len - col_info.int_len;

   if (buffer.size())   pad_r(UNI_PAD_L3, diff);
   else                 buffer.push_back(UCS_string(diff, UNI_PAD_L3));

   col_info.int_len = COL_INFO.int_len;

   Log(LOG_printbuf_align)   debug(CERR, "after align_left()");
}
//-----------------------------------------------------------------------------
bool
PrintBuffer::is_rectangular() const
{
   if (get_height())
      {
        const ShapeItem w = get_width(0);
        loop(h, get_height())
           {
             if (get_width(h) != w)    return false;
           }
      }

   return true;
}
//-----------------------------------------------------------------------------
void
ColInfo::consider(ColInfo & other)
{
   flags |= other.flags;
   if (other.imag_len)   flags |= has_j;

const int expo_len = real_len - fract_len - int_len;
const int expo_len1 = other.real_len - other.fract_len - other.int_len;

   if (int_len   < other.int_len)
      {
        real_len += other.int_len - int_len;
           int_len = other.int_len;
      }

   if (fract_len < other.fract_len)
      {
        real_len += other.fract_len - fract_len;
        fract_len = other.fract_len;
      }

   real_len = int_len + fract_len + expo_len;
   if (expo_len < expo_len1)    real_len += expo_len1 - expo_len;

   if (imag_len  < other.imag_len)    imag_len  = other.imag_len;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const PrintBuffer & pb)
{
   out << endl;   return pb.debug(out);
}
//-----------------------------------------------------------------------------
