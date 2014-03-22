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

#include <stdlib.h>

#include "Quad_RL.hh"
#include "Value.hh"
#include "IntCell.hh"

//=============================================================================
Quad_RL::Quad_RL()
   : SystemVariable(ID_QUAD_RL)
{
Value_P value(new Value(LOC));

   new (&value->get_ravel(0)) IntCell(1);
   value->check_value(LOC);

   srandom(1);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_RL::assign(Value_P value, const char * loc)
{
   if (!value->is_skalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_int_value();

   srandom(val);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
APL_Integer
Quad_RL::get_random()
{
const APL_Integer r1 = random();
const APL_Integer seed = random();
   Assert(value_stack.size());
   if (value_stack.back().name_class != NC_VARIABLE)   VALUE_ERROR;

   new (&value_stack.back().apl_val->get_ravel(0))   IntCell(seed);
   return seed << 32 | r1;
}
//=============================================================================
