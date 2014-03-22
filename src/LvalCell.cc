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

#include "LvalCell.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"

//-----------------------------------------------------------------------------
Cell *
LvalCell::get_lval_value()  const
{
  return value.lval;
}
//-----------------------------------------------------------------------------
PrintBuffer
LvalCell::character_representation(const PrintContext & pctx) const
{
   if (!value.lval)
      {
        UTF8_string utf("(Cell * 0)");
        UCS_string ucs(utf);
        ColInfo ci;
        PrintBuffer pb(ucs, ci);
        return pb;
      }

PrintBuffer pb = value.lval->character_representation(pctx);
   pb.pad_l(Unicode('='), 1);
   pb.pad_r(Unicode('='), 1);
   return pb;
}
//-----------------------------------------------------------------------------


