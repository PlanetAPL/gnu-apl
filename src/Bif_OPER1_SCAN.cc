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

#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "LvalCell.hh"
#include "Workspace.hh"

Bif_OPER1_SCAN     Bif_OPER1_SCAN::fun;
Bif_OPER1_SCAN1    Bif_OPER1_SCAN1::fun;

//-----------------------------------------------------------------------------
Token
Bif_SCAN::expand(Value_P A, Value_P B, Axis axis)
{
   // turn skalar B into ,B
   //
Shape shape_B = B->get_shape();
   if (shape_B.get_rank() == 0)
      {
         shape_B.add_shape_item(1);
         axis = 0;
      }
   if (axis >= shape_B.get_rank())   INDEX_ERROR;

   if (A->get_rank() > 1)            RANK_ERROR;
   if (shape_B.get_rank() <= axis)   RANK_ERROR;

const ShapeItem ec_A = A->element_count();
const APL_Float qct = Workspace::get_CT();
ShapeItem ones_A = 0;
vector<ShapeItem> rep_counts;
   rep_counts.reserve(ec_A);
   loop(a, ec_A)
      {
        APL_Integer rep_A = A->get_ravel(a).get_near_int(qct);
        rep_counts.push_back(rep_A);
        if      (rep_A == 0)        ;
        else if (rep_A == 1)        ++ones_A;
        else                        DOMAIN_ERROR;
      }

Shape shape_Z(shape_B);
   shape_Z.set_shape_item(axis, ec_A);
Value_P Z(new Value(shape_Z, LOC));

   if (ec_A == 0)   // (⍳0)/B : 
      {
        if (shape_B.get_shape_item(axis) > 1)   LENGTH_ERROR;

        Z->set_default(*B.get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_Z3(shape_Z, axis);

const Cell * cB = &B->get_ravel(0);
const bool lval = cB->is_lval_cell();

ShapeItem inc_1 = shape_Z3.l();   // increment after result l items
ShapeItem inc_2 = 0;              // increment after result m*l items

   if (B->is_skalar() || (shape_B.get_shape_item(axis) == 1
                      && (shape_Z3.l() != 1)))
      {
         inc_1 = 0;
         inc_2 = shape_Z3.l();
      }
   else if (ones_A != shape_B.get_shape_item(axis))   LENGTH_ERROR;

   loop(h, shape_Z3.h())
      {
        const Cell * fill = cB;
        loop(m, rep_counts.size())
           {
             if (rep_counts[m] == 1)   // copy items from B
                {
                  loop(l, shape_Z3.l())   Z->next_ravel()->init(cB[l]);
                  cB += inc_1;
                }
             else                      // init items
                {
                  if (lval)
                     {
                       loop(l, shape_Z3.l())   new (Z->next_ravel()) LvalCell(0);
                     }
                  else
                     {
                       loop(l, shape_Z3.l())   Z->next_ravel()->init_type(fill[l]);
                     }
                }
           }

        cB += inc_2;
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_SCAN::scan(Function * LO, Value_P B, Axis axis)
{
   Assert(LO);
   if (!LO->has_result())   DOMAIN_ERROR;

   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (axis >= B->get_rank())   INDEX_ERROR;

const ShapeItem m_len = B->get_shape_item(axis);

   if (m_len == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (m_len == 1)
      {
        const Shape shape_Z = B->get_shape().without_axis(axis);
        return Bif_F12_RHO::do_reshape(shape_Z, B);
      }

const Shape3 Z3(B->get_shape(), axis);

Value_P Z(new Value(B->get_shape(), LOC));
EOC_arg arg(Z, B);
REDUCTION & _arg = arg.u.u_REDUCTION;

   _arg.init(Z3, LO, &B->get_ravel(0), m_len, 1, 1);

Token tok(TOK_FIRST_TIME);
   Bif_REDUCE::eoc_beam(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN::eval_AXB(Value_P A,
                             Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());

   return expand(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN::eval_LXB(Token & LO, Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());
   return scan(LO.get_function(), B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN1::eval_AXB(Value_P A,
                             Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());

   return expand(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_SCAN1::eval_LXB(Token & LO, Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());
   return scan(LO.get_function(), B, axis);
}
//-----------------------------------------------------------------------------
