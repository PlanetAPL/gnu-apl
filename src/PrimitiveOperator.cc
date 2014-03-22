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

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "IndexIterator.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "PointerCell.hh"
#include "PrimitiveOperator.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Token
PrimitiveOperator::fill(const Shape shape_Z, Value_P A, Function * fun,
                        Value_P B, const char * loc)
{
Value_P Fill_A;   // argument A of the fill function
Value_P Fill_B;   // argument B of the fill function

   if (A->is_empty())   Fill_A = A->prototype(LOC);
   else                 Fill_A = Bif_F12_TAKE::fun.eval_B(A).get_apl_val();

   if (B->is_empty())   Fill_B = B->prototype(LOC);
   else                 Fill_B = Bif_F12_TAKE::fun.eval_B(B).get_apl_val();

Token tok = fun->eval_fill_AB(Fill_A, Fill_B);

   if (tok.get_Class() != TC_VALUE)   return tok;

Value_P Z = tok.get_apl_val();
Value_P Z1(new Value(shape_Z, LOC));   // shape_Z is empty
   Z1->get_ravel(0).  init_from_value(Z, loc);

   Z1->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z1);
}
//-----------------------------------------------------------------------------
