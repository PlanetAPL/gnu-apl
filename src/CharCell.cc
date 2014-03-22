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

#include "Value.hh"
#include "Error.hh"
#include "Workspace.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "PointerCell.hh"

//-----------------------------------------------------------------------------
bool
CharCell::greater(const Cell * other, bool ascending) const
{
const uint64_t this_val  = get_char_value();

   switch(other->get_cell_type())
      {
        case CT_NONE:
        case CT_BASE:
             Assert(0);

        case CT_CHAR:
             {
               const Unicode other_val = other->get_char_value();
               if (this_val == other_val)
                  return this > other ? ascending : !ascending;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_INT:
             {
               const APL_Integer other_val = other->get_int_value();
               if (this_val == other_val)   return ascending;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other->get_real_value();
               if (this_val == other_val)   return true;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_COMPLEX:
        case CT_POINTER:
             DOMAIN_ERROR;

        default:
           Assert(0);
      }
}
//-----------------------------------------------------------------------------
bool
CharCell::equal(const Cell & other, APL_Float qct) const
{
   if (!other.is_character_cell())   return false;
   return value.aval == other.get_char_value();
}
//-----------------------------------------------------------------------------
void
CharCell::bif_equal(Cell * Z, const Cell * A) const
{
   if (!A->is_character_cell())                  new (Z) IntCell(0);
   else if (A->get_char_value() == value.aval)   new (Z) IntCell(1);
   else                                          new (Z) IntCell(0);
}
//-----------------------------------------------------------------------------
bool
CharCell::is_example_field() const
{
   if (value.aval == UNI_ASCII_COMMA)       return true;
   if (value.aval == UNI_ASCII_FULLSTOP)    return true;
   return value.aval >= UNI_ASCII_0 && value.aval <= UNI_ASCII_9;
}
//-----------------------------------------------------------------------------
PrintBuffer
CharCell::character_representation(const PrintContext & pctx) const
{
UCS_string ucs;
ColInfo info;
   info.flags |= CT_CHAR;

   if (pctx.get_style() == PR_APL_FUN)
      {
        Unicode uni = get_char_value();
        if (uni == UNI_SINGLE_QUOTE)
           {
             ucs.append(UNI_SINGLE_QUOTE);
             ucs.append(uni);
             ucs.append(uni);
             ucs.append(UNI_SINGLE_QUOTE);

             info.int_len = 3;
           }
        else
           {
             ucs.append(UNI_SINGLE_QUOTE);
             ucs.append(uni);
             ucs.append(UNI_SINGLE_QUOTE);

             info.int_len = 3;
           }
      }
   else
      {
        ucs.append(get_char_value());

        info.int_len = 1;
      }


   info.real_len = info.int_len;

   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
int
CharCell::CDR_size() const
{
   // use 1 byte for small chars and 4 bytes for other UNICODE chars
   //
const Unicode uni = get_char_value();
   if (uni <  0)     return 4;
   if (uni >= 256)   return 4;
   return 1;
}
//-----------------------------------------------------------------------------
