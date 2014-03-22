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

#include "SkalarFunction.hh"
#include "Value.hh"
#include "Workspace.hh"
#include "ArrayIterator.hh"
#include "IndexIterator.hh"
#include "IndexExpr.hh"
#include "Avec.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "PointerCell.hh"

Bif_F2_LESS       Bif_F2_LESS::fun;
Bif_F2_EQUAL      Bif_F2_EQUAL::fun;
Bif_F2_FIND       Bif_F2_FIND::fun;
Bif_F2_GREATER    Bif_F2_GREATER::fun;
Bif_F2_AND        Bif_F2_AND::fun;
Bif_F2_OR         Bif_F2_OR::fun;
Bif_F2_LEQ        Bif_F2_LEQ::fun;
Bif_F2_MEQ        Bif_F2_MEQ::fun;
Bif_F2_UNEQ       Bif_F2_UNEQ::fun;
Bif_F2_NOR        Bif_F2_NOR::fun;
Bif_F2_NAND       Bif_F2_NAND::fun;
Bif_F12_PLUS      Bif_F12_PLUS::fun;
Bif_F12_POWER     Bif_F12_POWER::fun;
Bif_F12_BINOM     Bif_F12_BINOM::fun;
Bif_F12_MINUS     Bif_F12_MINUS::fun;
Bif_F12_ROLL      Bif_F12_ROLL::fun;
Bif_F12_TIMES     Bif_F12_TIMES::fun;
Bif_F12_DIVIDE    Bif_F12_DIVIDE::fun;
Bif_F12_CIRCLE    Bif_F12_CIRCLE::fun;
Bif_F12_RND_UP    Bif_F12_RND_UP::fun;
Bif_F12_RND_DN    Bif_F12_RND_DN::fun;
Bif_F12_STILE     Bif_F12_STILE::fun;
Bif_F12_LOGA      Bif_F12_LOGA::fun;
Bif_F12_WITHOUT   Bif_F12_WITHOUT::fun;

//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_skalar_B(Value_P B, prim_f1 fun)
{
const ShapeItem count = B->element_count();
   if (count == 0)   return eval_fill_B(B);

Value_P Z(new Value(B->get_shape(), LOC));
   loop(c, count)
       {
         const Cell * cell_B =  &B->get_ravel(c);
         Cell * cell_Z = &Z->get_ravel(c);
         if (cell_B->is_pointer_cell())
            {
              Token token = eval_skalar_B(cell_B->get_pointer_value(), fun);
              new (cell_Z) PointerCell(token.get_apl_val());
            }
         else
            {
              (cell_B->*fun)(cell_Z);
            }
       }

   if (count == 0)   // Z was empty (hence B was empty)
      {
        const Cell & cB = B->get_ravel(0);
        if (cB.is_pointer_cell())   Z->get_ravel(0).init(cB);
        else                        new (&Z->get_ravel(0)) IntCell(0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
SkalarFunction::expand_pointers(Cell * cell_Z, const Cell * cell_A,
                                const Cell * cell_B, prim_f2 fun)
{
   if (cell_A->is_pointer_cell())
      {
        if (cell_B->is_pointer_cell())   // A and B are both pointers
           {
             Value_P value_A = cell_A->get_pointer_value();
             Value_P value_B = cell_B->get_pointer_value();
             Token token = eval_skalar_AB(value_A, value_B, fun);
             new (cell_Z) PointerCell(token.get_apl_val());
           }
        else                             // A is pointer, B is simple
           {
             Value_P value_A = cell_A->get_pointer_value();
             Value_P skalar_B(new Value(*cell_B, LOC));
             Token token = eval_skalar_AB(value_A, skalar_B, fun);
             new (cell_Z) PointerCell(token.get_apl_val());
           }
      }
   else                                  // A is simple
      {
        if (cell_B->is_pointer_cell())   // A is simple, B is pointer
           {
             Value_P skalar_A(new Value(*cell_A, LOC));
             Value_P value_B = cell_B->get_pointer_value();
             Token token = eval_skalar_AB(skalar_A, value_B, fun);
             new (cell_Z) PointerCell(token.get_apl_val());
           }
   else                                // A and B are both plain
         {
           (cell_B->*fun)(cell_Z, cell_A);
         }
      }
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_skalar_AB(Value_P A, Value_P B, prim_f2 fun)
{
   if (A->is_skalar_or_len1_vector())
      {
        const ShapeItem count = B->element_count();
        if (count == 0)   return eval_fill_AB(A, B);

        const Cell * cell_A = &A->get_ravel(0);
        Value_P Z(new Value(B->get_shape(), LOC));

        loop(c, count)
            {
              const Cell * cell_B = &B->get_ravel(c);
              Cell * cell_Z = &Z->get_ravel(c);
              expand_pointers(cell_Z, cell_A, cell_B, fun);
            }

        Z->set_default(*B.get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->is_skalar_or_len1_vector())
      {
        const ShapeItem count = A->element_count();
        if (count == 0)   return eval_fill_AB(A, B);

        const Cell * cell_B = &B->get_ravel(0);
        Value_P Z(new Value(A->get_shape(), LOC));

        loop(c, count)
            {
              const Cell * cell_A = &A->get_ravel(c);
              Cell * cell_Z = &Z->get_ravel(c);
              expand_pointers(cell_Z, cell_A, cell_B, fun);
            }

        Z->set_default(*B.get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (!A->same_shape(B))
      {
        if (!A->same_rank(B))   RANK_ERROR;
        else                    LENGTH_ERROR;
      }

const ShapeItem count = A->element_count();
   if (count == 0)   return eval_fill_AB(A, B);

Value_P Z(new Value(A->get_shape(), LOC));

   loop(c, count)
       {
         const Cell * cell_A = &A->get_ravel(c);
         const Cell * cell_B = &B->get_ravel(c);
         Cell * cell_Z = &Z->get_ravel(c);
         expand_pointers(cell_Z, cell_A, cell_B, fun);
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_fill_AB(Value_P A, Value_P B)
{
   // eval_fill_AB() is called when A or B or both are empty
   //
   if (A->is_skalar())   // then B is empty
      {
        Value_P Z = B->clone(LOC);
        Z->to_proto();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->is_skalar())   // then A is empty
      {
        Value_P Z = A->clone(LOC);
        Z->to_proto();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // both A and B are empty
   //
   Assert(A->same_shape(B));   // has been checked already
Value_P Z = B->clone(LOC);
   Z->to_proto();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);

//   return Bif_F2_UNEQ::fun.eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_fill_B(Value_P B)
{
   // eval_fill_B() is called when a scalar function with empty B is called
   //
Value_P Z = B->clone(LOC);
   Z->to_proto();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_skalar_identity_fun(Value_P B, Axis axis, Value_P FI0)
{
   // for skalar functions the result of the identity function for skalar
   // function F is defined as follows (lrm p. 210)
   //
   // Z←SRρB+F/ι0    with SR ↔ ⍴Z
   //
   // The term F/ι0 is passed as argument FI0, so that the above becomes
   //
   // Z←SRρB+FI0
   //
   // Since F is skalar, the ravel elements of B (if any) are 0 and
   // therefore B+FI0 becomes (⍴B)⍴FI0.
   //
   if (!FI0->is_skalar())   Q(FI0->get_shape())

const Shape shape_Z = B->get_shape().without_axis(axis);

Value_P Z(new Value(shape_Z, LOC));

const Cell & proto_B = B->get_ravel(0);
const Cell & cell_FI0 = FI0->get_ravel(0);

   if (proto_B.is_pointer_cell())
      {
        // create a value like ↑B but with all ravel elements like FI0...
        //
        Value_P sub(new Value(proto_B.get_pointer_value()->get_shape(), LOC));
        const ShapeItem len_sub = sub->nz_element_count();
        Cell * csub = &sub->get_ravel(0);
        loop(s, len_sub)   csub++->init(cell_FI0);

        while (Z->more())   new (Z->next_ravel()) PointerCell(sub->clone(LOC));
      }
   else
      {
        while (Z->more())   Z->next_ravel()->init(cell_FI0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_skalar_AXB(Value_P A, Value_P X,
                                 Value_P B, prim_f2 fun)
{
   if (A->is_skalar_or_len1_vector() || B->is_skalar_or_len1_vector() || !&X)
      return eval_skalar_AB(A, B, fun);

   if (X->get_rank() > 1)   INDEX_ERROR;

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();
const Rank rank_A = A->get_rank();
const Rank rank_B = B->get_rank();
bool axis_present[MAX_RANK];
   loop(r, MAX_RANK)   axis_present[r] = false;

const ShapeItem len_X = X->element_count();
   loop(il, len_X)
       {
         APL_Integer i = A->get_ravel(il).get_near_int(qct) - qio;
         if (i < 0)                        AXIS_ERROR;
         if (i >= rank_A && i >= rank_B)   AXIS_ERROR;
         if (axis_present[i])              AXIS_ERROR;
         axis_present[i] = true;
       }

   if (rank_A < rank_B)
      return eval_skalar_AXB(A, axis_present, B, fun, false);
   return eval_skalar_AXB(B, axis_present, A, fun, true);
}
//-----------------------------------------------------------------------------
Token
SkalarFunction::eval_skalar_AXB(Value_P A, bool * axis_present,
                                 Value_P B, prim_f2 fun, bool reversed)
{
   // create a weight vector for A from B's weight vector with axes that
   // are present in b axis_present removed.
   //
Shape weight = B->get_shape().reverse_scan();

   {
     Rank ra = 0;
     loop(r, weight.get_rank())
         {
            if (axis_present[r])                weight.set_shape_item(r, 0);
            else if (B->get_shape_item(r) !=
                     A->get_shape_item(ra++))   INDEX_ERROR;
         }
   }

Value_P Z(new Value(B->get_shape(), LOC));

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it(B->get_shape()); !it.done(); ++it)
       {
         ShapeItem a = 0;
         loop(r, B->get_rank())   a += weight.get_shape_item(r)
                                     * it.get_value(r);

         const Cell * cA = &A->get_ravel(a);

         if (reversed)   expand_pointers(Z->next_ravel(), cB++, &A->get_ravel(a), fun);
         else            expand_pointers(Z->next_ravel(), &A->get_ravel(a), cB++, fun);
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Bif_F2_FIND::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
Value_P Z(new Value(B->get_shape(), LOC));

const ShapeItem len_Z = Z->element_count();
Shape shape_A;

   if (A->get_rank() > B->get_rank())   // then Z is all zeros.
      {
        loop(z, len_Z)   new (&Z->get_ravel(z))   IntCell(0);
        goto done;
      }

   // Reshape A to match rank B if necessary...
   //
   {
     const Rank rank_diff = B->get_rank() - A->get_rank();
     loop(d, rank_diff)       shape_A.add_shape_item(1);
     loop(r, A->get_rank())   shape_A.add_shape_item(A->get_shape_item(r));
   }

   // if any dimension of A is longer than that of B, then A cannot be found.
   //
   loop(r, B->get_rank())
       {
         if (shape_A.get_shape_item(r) > B->get_shape_item(r))
            {
              loop(z, len_Z)   new (&Z->get_ravel(z))   IntCell(0);
              goto done;
            }
       }

   for (ArrayIterator zi(B->get_shape()); !zi.done(); ++zi)
       {
         if (contained(shape_A, &A->get_ravel(0), B, zi.get_values(), qct))
            new (&Z->get_ravel(zi.get_total()))   IntCell(1);
         else
            new (&Z->get_ravel(zi.get_total()))   IntCell(0);
       }

done:
   Z->set_default(*Value::Zero_P);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
bool
Bif_F2_FIND::contained(const Shape & shape_A, const Cell * cA,
                       Value_P B, const Shape & idx_B, double qct)
{
   loop(r, B->get_rank())
       {
         if ((idx_B.get_shape_item(r) + shape_A.get_shape_item(r))
             > B->get_shape_item(r))    return false;
       }

const Shape weight = B->get_shape().reverse_scan();

   for (ArrayIterator ai(shape_A); !ai.done(); ++ai)
       {
         const Shape & pos_A = ai.get_values();
         ShapeItem pos_B = 0;
         loop(r, B->get_rank())   pos_B += weight.get_shape_item(r)
                                         * (idx_B.get_shape_item(r)
                                         + pos_A.get_shape_item(r));

         if (!cA[ai.get_total()].equal(B->get_ravel(pos_B), qct))
            return false;
       }

   return true;
}
//=============================================================================
Token
Bif_F12_ROLL::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   // draw A items  from the set [quad-IO ... B]
   //
   if (!A->is_skalar_or_len1_vector())   RANK_ERROR;
   if (!B->is_skalar_or_len1_vector())   RANK_ERROR;

const uint32_t aa = A->get_ravel(0).get_near_int(qct);
APL_Integer set_size = B->get_ravel(0).get_near_int(qct);
   if (aa > set_size)   DOMAIN_ERROR;
   if (set_size <= 0)   DOMAIN_ERROR;

Value_P Z(new Value(aa, LOC));

uint32_t idx_B[set_size];
   loop(c, set_size)   idx_B[c] = c + qio;

   loop(z, aa)
       {
         const APL_Integer rnd = Workspace::get_RL() % set_size;
         new (&Z->get_ravel(z)) IntCell(idx_B[rnd]);
         idx_B[rnd] = idx_B[set_size - 1];   // move last item in.
         --set_size;
       }

   Z->set_default(*Value::Zero_P);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROLL::eval_B(Value_P B)
{
   // the standard wants ? to be atomic. We therefore check beforehand
   // that all elements of B are proper, and throw an error if not
   //
const ShapeItem ec_B = B->element_count();
const APL_Float qct = Workspace::get_CT();

   loop(b, ec_B)
      {
        // throw error now !
        const APL_Integer val =  B->get_ravel(b).get_near_int(qct);
        if (val <= 0)   DOMAIN_ERROR;
      }

   return eval_skalar_B(B, &Cell::bif_roll);
}
//=============================================================================
Token
Bif_F12_WITHOUT::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const uint32_t len_A = A->element_count();
const uint32_t len_B = B->element_count();
const APL_Float qct = Workspace::get_CT();
Value_P Z(new Value(len_A, LOC));

uint32_t len_Z = 0;

   loop(a, len_A)
      {
        bool found = false;
        const Cell & cell_A = A->get_ravel(a);
        loop(b, len_B)
            {
              if (cell_A.equal(B->get_ravel(b), qct))
                 {
                   found = true;
                   break;
                 }
            }

        if (!found)
           {
             Z->get_ravel(len_Z++).init(cell_A);
           }
      }

   Z->set_shape_item(0, len_Z);

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================

