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

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_OPER1_EACH.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "CollatingCache.hh"
#include "FloatCell.hh"
#include "Id.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "StateIndicator.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"
                                            
Bif_F0_ZILDE      Bif_F0_ZILDE::fun;         // ⍬
Bif_F1_EXECUTE    Bif_F1_EXECUTE::fun;       // ⍎
Bif_F2_INDEX      Bif_F2_INDEX::fun;         // ⌷
Bif_F12_PARTITION Bif_F12_PARTITION::fun;    // ⊂
Bif_F12_PICK      Bif_F12_PICK::fun;         // ⊃
Bif_F12_COMMA     Bif_F12_COMMA::fun;        // ,
Bif_F12_COMMA1    Bif_F12_COMMA1::fun;       // ⍪
Bif_F12_TAKE      Bif_F12_TAKE::fun;         // ↑
Bif_F12_DROP      Bif_F12_DROP::fun;         // ↓
Bif_F12_ELEMENT   Bif_F12_ELEMENT::fun;      // ∈
Bif_F12_EQUIV     Bif_F12_EQUIV::fun;        // ≡
Bif_F12_ENCODE    Bif_F12_ENCODE::fun;       // ⊤
Bif_F12_DECODE    Bif_F12_DECODE::fun;       // ⊥
Bif_F12_DOMINO    Bif_F12_DOMINO::fun;       // ⌹
Bif_F12_ROTATE    Bif_F12_ROTATE::fun;       // ⌽
Bif_F12_ROTATE1   Bif_F12_ROTATE1::fun;      // ⊖
Bif_F12_TRANSPOSE Bif_F12_TRANSPOSE::fun;    // ⍉
Bif_F12_SORT_ASC  Bif_F12_SORT_ASC::fun;     // ⍋
Bif_F12_SORT_DES  Bif_F12_SORT_DES::fun;     // ⍒
Bif_F12_FORMAT    Bif_F12_FORMAT::fun;       // ⍕
Bif_F12_INDEX_OF  Bif_F12_INDEX_OF::fun;     // ⍳
Bif_F12_RHO       Bif_F12_RHO::fun;          // ⍴
Bif_F1_UNIQUE     Bif_F1_UNIQUE::fun;        // ∪
Bif_F2_LEFT       Bif_F2_LEFT::fun;          // ⊣
Bif_F2_RIGHT      Bif_F2_RIGHT::fun;         // ⊢

const CharCell PrimitiveFunction::c_filler(UNI_ASCII_SPACE);
const IntCell  PrimitiveFunction::n_filler(0);

//-----------------------------------------------------------------------------
Token
PrimitiveFunction::eval_fill_AB(Value_P A, Value_P B)
{
   return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
ostream &
PrimitiveFunction::print(ostream & out) const
{
   return out << get_Id();
}
//-----------------------------------------------------------------------------
void
PrimitiveFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "System Function ";
   print(out);
   out << endl;
}
//=============================================================================
Token
Bif_F0_ZILDE::eval_()
{
   return Token(TOK_APL_VALUE1, Value::Idx0_P);
}
//=============================================================================
Token
Bif_F12_RHO::eval_B(Value_P B)
{
Value_P Z(new Value(B->get_rank(), LOC));

   loop(r, B->get_rank())   new (&Z->get_ravel(r)) IntCell(B->get_shape_item(r));

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::eval_AB(Value_P A, Value_P B)
{
const Shape shape_Z(A, Workspace::get_CT(), 0);

   return do_reshape(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::do_reshape(const Shape & shape_Z, Value_P B)
{
Value_P Z(new Value(shape_Z, LOC));

const ShapeItem len_B = B->element_count();
const ShapeItem len_Z = Z->element_count();

   if (len_B == 0)   // empty B: use prototype
      {
        loop(z, len_Z)   Z->get_ravel(z).init_type(B->get_ravel(0));
      }
   else
      {
        loop(z, len_Z)   Z->get_ravel(z).init(B->get_ravel(z % len_B));
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_INDEX_OF::eval_B(Value_P B)
{
   // interval

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!B->is_skalar_or_len1_vector())
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                    LENGTH_ERROR;
      }

const Cell * cell = &B->get_ravel(0);
const APL_Integer len = cell->get_near_int(qct);

Value_P Z(new Value(len, LOC));

   loop(z, len)   new (&Z->get_ravel(z)) IntCell(qio + z);

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_INDEX_OF::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   // Index of
   //
   if (!A->is_skalar_or_vector())   RANK_ERROR;

const uint64_t len_A  = A->element_count();
const uint64_t len_BZ = B->element_count();

Value_P Z(new Value(B->get_shape(), LOC));

   loop(bz, len_BZ)
       {
         const Cell & cell_B = B->get_ravel(bz);
         bool found = false;
         loop(a, len_A)
             {
               if (cell_B.equal(A->get_ravel(a), qct))
                  {
                    new (&Z->get_ravel(bz)) IntCell(qio + a);
                    found = true;
                    break;
                  }
             }
         if (!found)
            new (&Z->get_ravel(bz)) IntCell(qio + len_A);
       }

   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::ravel(const Shape & new_shape, Value_P B)
{
Value_P Z(new Value(new_shape, LOC));

const ShapeItem count = B->element_count();
   Assert(count == Z->element_count());

   loop(c, count)   Z->get_ravel(c).init(B->get_ravel(c));

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_COMMA::prepend_skalar(const Cell & cell_A, Axis axis, Value_P B)
{
   if (B->is_skalar())
      {
        Value_P Z(new Value(2, LOC));
        Z->get_ravel(0).init(cell_A);
        Z->get_ravel(1).init(B->get_ravel(0));
        Z->check_value(LOC);
        return Z;
      }

   if (axis >= B->get_rank())   INDEX_ERROR;

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_B3(B->get_shape(), axis);
   const uint32_t slice_a = shape_B3.l();
   const uint32_t slice_b = shape_B3.l() * B->get_shape_item(axis);

const Cell * cB = &B->get_ravel(0);

   loop(hz, shape_B3.h())
       {
         loop(lz, slice_a)   Z->next_ravel()->init(cell_A);

         Cell::copy(*Z.get(), cB, slice_b);
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_COMMA::append_skalar(Value_P A, Axis axis, const Cell & cell_B)
{
   if (axis >= A->get_rank())   INDEX_ERROR;

Shape shape_Z(A->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_A3(A->get_shape(), axis);
const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
const uint32_t slice_b = shape_A3.l();

const Cell * cA = &A->get_ravel(0);

   loop(hz, shape_A3.h())
       {
         Cell::copy(*Z.get(), cA, slice_a);
         loop(lz, slice_b)   Z->next_ravel()->init(cell_B);
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::catenate(Value_P A, Axis axis, Value_P B)
{
   if (A->is_skalar())
      {
        const Cell & cell_A = A->get_ravel(0);
        Value_P Z = prepend_skalar(cell_A, axis, B);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->is_skalar())
      {
        const Cell & cell_B = B->get_ravel(0);
        Value_P Z = append_skalar(A, axis, cell_B);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if ((A->get_rank() + 1) == B->get_rank())
      {
        Shape shape_Z;

        // check shape conformance.
        {
          ShapeItem ra = 0;
          loop(rb, B->get_rank())
             {
               if (rb != axis)
                  {
                    shape_Z.add_shape_item(B->get_shape_item(rb));
                    if (A->get_shape_item(ra) != B->get_shape_item(rb))
                       {
                         LENGTH_ERROR;
                       }
                    ++ra;
                  }
               else
                  {
                    shape_Z.add_shape_item(B->get_shape_item(rb) + 1);
                  }
             }
        }

        Value_P Z(new Value(shape_Z, LOC));

        Z->set_default(*B.get());

        const Shape3 shape_B3(B->get_shape(), axis);
        const uint32_t slice_a = shape_B3.l();
        const uint32_t slice_b = shape_B3.l() * B->get_shape_item(axis);

        const Cell * cA = &A->get_ravel(0);
        const Cell * cB = &B->get_ravel(0);

        loop(hz, shape_B3.h())
            {
              Cell::copy(*Z.get(), cA, slice_a);
              Cell::copy(*Z.get(), cB, slice_b);
            }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (A->get_rank() == (B->get_rank() + 1))
      {
        // e.g.	        ∆∆∆ , 3
        //              ∆∆∆   4
        //
        Shape shape_Z;

        // check shape conformance. We step ranks ra and rb, except for the
        // axis where only ra is incremented.
        {
          uint32_t rb = 0;
          loop(ra, A->get_rank())
             {
               if (ra != axis)
                  {
                    if (A->get_shape_item(ra) != B->get_shape_item(rb))
                       LENGTH_ERROR;

                    shape_Z.add_shape_item(A->get_shape_item(ra));
                    ++rb;
                  }
               else
                  {
                    shape_Z.add_shape_item(A->get_shape_item(ra) + 1);
                  }
             }
        }

        Value_P Z(new Value(shape_Z, LOC));

        Z->set_default(*B.get());

        const Shape3 shape_A3(A->get_shape(), axis);
        const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
        const uint32_t slice_b = shape_A3.l();

        const Cell * cA = &A->get_ravel(0);
        const Cell * cB = &B->get_ravel(0);

        loop (hz, shape_A3.h())
            {
              Cell::copy(*Z.get(), cA, slice_a);
              Cell::copy(*Z.get(), cB, slice_b);
            }

        Z->set_default(*B.get());

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (A->get_rank() != B->get_rank())   RANK_ERROR;

   // A and B have the same rank. The shapes need to agree, except for the
   // dimension corresponding to the axis.
   //
Shape shape_Z;

   loop(r, A->get_rank())
       {
         if (r != axis)
            {
              if (A->get_shape_item(r) != B->get_shape_item(r))
                 LENGTH_ERROR;

              shape_Z.add_shape_item(A->get_shape_item(r));
            }
         else
            {
              shape_Z.add_shape_item(A->get_shape_item(r) +
                                   + B->get_shape_item(r));
            }
       }

Value_P Z(new Value(shape_Z, LOC));

   Z->set_default(*B.get());

const Shape3 shape_A3(A->get_shape(), axis);

const Cell * cA = &A->get_ravel(0);
const Cell * cB = &B->get_ravel(0);
const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
const uint32_t slice_b = shape_A3.l() * B->get_shape_item(axis);

   loop(hz, shape_A3.h())
       {
         Cell::copy(*Z.get(), cA, slice_a);
         Cell::copy(*Z.get(), cB, slice_b);
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::laminate(Value_P A, Axis axis, Value_P B)
{
   // shapes of A and B must be the same, unless one of them is a skalar.
   //
   if (!A->is_skalar() && !B->is_skalar())
      A->get_shape().check_same(B->get_shape(), E_INDEX_ERROR, E_LENGTH_ERROR, LOC);

const Shape shape_Z = A->is_skalar() ? B->get_shape().insert_axis(axis, 2)
                                    : A->get_shape().insert_axis(axis, 2);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_Z3(shape_Z, axis);
   Assert(shape_Z3.m() == 2);

const Cell * cA = &A->get_ravel(0);
const Cell * cB = &B->get_ravel(0);

   if (A->is_skalar())
      {
        if (B->is_skalar())
           {
              Z->next_ravel()->init(*cA);
              Z->next_ravel()->init(*cB);
           }
        else
           {
             loop(h, shape_Z3.h())
                 {
                   loop(l, shape_Z3.l())   Z->next_ravel()->init(*cA);
                   Cell::copy(*Z.get(), cB, shape_Z3.l());
                }
           }
      }
   else
      {
        if (B->is_skalar())
           {
             loop(h, shape_Z3.h())
                 {
                   Cell::copy(*Z.get(), cA, shape_Z3.l());
                   loop(l, shape_Z3.l())   Z->next_ravel()->init(*cB);
                }
           }
        else
           {
             loop(h, shape_Z3.h())
                 {
                   Cell::copy(*Z.get(), cA, shape_Z3.l());
                   Cell::copy(*Z.get(), cB, shape_Z3.l());
                }
           }
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::ravel_axis(Value_P X, Value_P B, Axis axis)
{
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!&X)   SYNTAX_ERROR;

   // I must be a (integer or real) skalar or simple integer vector
   //
   if (X->get_rank() > 1)   INDEX_ERROR;

   // There are 3 variants, determined by I:
   //
   // 1) I is empty:             append a new last axis of length 1
   // 2) I is a fraction:        append a new axis before I
   // 3a) I is integer skalar:   return B
   // 3b) I is integer vector:   combine axes

   // case 1:   ,['']B or ,[⍳0]B : append new first or last axis of length 1.
   //
   if (X->element_count() == 0)
      {
        if (B->get_rank() == MAX_RANK)   AXIS_ERROR;

        const Shape shape_Z = B->get_shape().insert_axis(axis, 1);
        return ravel(shape_Z, B);
      }

   // case 2:   ,[x.y]B : insert axis before axis x+1
   //
   if (!X->get_ravel(0).is_near_int(qct))  // fraction: insert an axis
      {
        if (B->get_rank() == MAX_RANK)   INDEX_ERROR;

        const APL_Float new_axis = X->get_ravel(0).get_real_value() - qio;
        const Axis axis((new_axis < 0) ? -1 : new_axis);
        const Shape shape_Z = B->get_shape().insert_axis(axis + 1, 1);
        return ravel(shape_Z, B);
      }

   // case 3a: ,[n]B : return B (combine single axis doesn't change anything)
   //
   if (X->is_skalar_or_len1_vector())   // single int: return B->
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   // case 3b: ,[n1 ... nk]B : combine axes.
   //
const Shape axes(X, qct, qio);

const ShapeItem from = axes.get_shape_item(0);
   if (from < 0)   AXIS_ERROR;

const ShapeItem to   = axes.get_last_shape_item();
   if (to > B->get_rank())   AXIS_ERROR;

const  ShapeItem len   = to - from;

   // check that the axes are contiguous and compute the number of elements
   // in the combined axes
   //
ShapeItem count = 1;
   loop(a, axes.get_rank())
      {
        if (axes.get_shape_item(a) != (from + a))   AXIS_ERROR;
        count *= B->get_shape_item(from + a);
      }

Shape shape_Z;
   loop (r, from)   shape_Z.add_shape_item(B->get_shape_item(r));
   shape_Z.add_shape_item(count);
   for (Rank r = to + 1; r < B->get_rank(); ++r)
       shape_Z.add_shape_item(B->get_shape_item(r));

   return ravel(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_B(Value_P B)
{
const Shape sh(B->element_count());
   return ravel(sh, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_AB(Value_P A, Value_P B)
{
Rank max_rank = A->get_rank();
   if (max_rank < B->get_rank())  max_rank = B->get_rank(); 
   return catenate(A, max_rank - 1, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   // catenate or laminate
   //
   if (!X->is_skalar_or_len1_vector())   AXIS_ERROR;

const Cell & cX = X->get_ravel(0);

const APL_Float qct = Workspace::get_CT();
   if (cX.is_near_int(qct))   // catenate along existing axis
      {
        const Axis axis = cX.get_checked_near_int() - Workspace::get_IO();
        if (axis < 0)                                       AXIS_ERROR;
        if (axis >= A->get_rank() && axis >= B->get_rank())   AXIS_ERROR;
        return catenate(A, axis, B);
      }

   return laminate(A, Axis(cX.get_real_value()) - Workspace::get_IO() + 1, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_B(Value_P B)
{
   // turn B into a matrix
   //
ShapeItem c1 = 1;   // assume B is skalar
ShapeItem c2 = 1;   // assume B is skalar;

   if (B->get_rank() >= 1)
      {
        c1 = B->get_shape_item(0);
        for (Rank r = 1; r < B->get_rank(); ++r)   c2 *= B->get_shape_item(r);
      }

Shape sh(c1, c2);
   return ravel(sh, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_AB(Value_P A, Value_P B)
{
   return catenate(A, 0, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   return Bif_F12_COMMA::fun.eval_AXB(A, X, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_B(Value_P B)
{
   if (B->is_skalar())
      {
        Value_P Z(new Value(LOC));

        B->get_ravel(0).bif_reciprocal(&Z->get_ravel(0));
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() == 1)   // inversion at the unit sphere
      {
        const APL_Float qct = Workspace::get_CT();
        const uint32_t len = B->get_shape_item(0);
        APL_Complex r2(0.0);
        loop(l, len)
            {
              const APL_Complex b = B->get_ravel(l).get_complex_value();
              r2 += b*b;
            }

        if (r2.real() < qct && r2.real() > -qct && 
            r2.imag() < qct && r2.imag() > -qct)
            DOMAIN_ERROR;
            
        Value_P Z(new Value(len, LOC));

        if (r2.imag() < qct && r2.imag() > -qct)   // real result
           {
             loop(l, len)
                 {
                   const APL_Float b = B->get_ravel(l).get_real_value();
                   new (&Z->get_ravel(l))   FloatCell(b / r2.real());
                 }
           }
        else                                       // complex result
           {
             loop(l, len)
                 {
                   const APL_Complex b = B->get_ravel(l).get_complex_value();
                   new (&Z->get_ravel(l))   ComplexCell(b / r2);
                 }
           }

        Z->set_default(*B.get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem rows = B->get_shape_item(0);
const ShapeItem cols = B->get_shape_item(1);
   if (cols > rows)   LENGTH_ERROR;

   // create an identity matrix I and call eval_AB(I, B).
   //
const Shape shape_I(rows, rows);
Value_P I(new Value(shape_I, LOC));

Cell * i = &I->get_ravel(0);
   loop(r, rows)
   loop(c, rows)
       new (i++) FloatCell((c == r) ? 1.0 : 0.0);

Token result = eval_AB(I, B);
   return result;
}
//-----------------------------------------------------------------------------
struct doublecomplex { double r; double i; };
extern Value_P divide_matrix(ShapeItem rows, ShapeItem cols_A, Value_P A,
                             ShapeItem cols_B, Value_P B,
                             const Shape & shape_Z, APL_Float qct);

Token
Bif_F12_DOMINO::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
ShapeItem rows_A = 1;
ShapeItem cols_A = 1;

Shape shape_Z;

   // if rank of A or B is < 2 then treat it as a 
   // 1 by n (or 1 by 1) matrix..
   //
ShapeItem rows_B = 1;
ShapeItem cols_B = 1;
   switch(B->get_rank())
      {
         case 0:  break;

         case 1:  rows_B = B->get_shape_item(0);
                  break;

         case 2:  cols_B = B->get_shape_item(1);
                  rows_B = B->get_shape_item(0);
                  shape_Z.add_shape_item(cols_B);
                  break;

         default: RANK_ERROR;
      }

   switch(A->get_rank())
      {
         case 0:  break;

         case 1:  rows_A = A->get_shape_item(0);
                  break;

         case 2:  cols_A = A->get_shape_item(1);
                  rows_A = A->get_shape_item(0);
                  shape_Z.add_shape_item(cols_A);
                  break;

         default: RANK_ERROR;
      }

   if (rows_B <  cols_B)   LENGTH_ERROR;
   if (rows_A != rows_B)   LENGTH_ERROR;

Value_P Z = divide_matrix(rows_A, cols_A, A, cols_B, B, shape_Z, qct);

   Z->set_default(*B.get());

   // round items close to integers
   //
   {
     ShapeItem ec_Z = shape_Z.get_volume();
     Cell * cZ = &Z->get_ravel(0);
     loop(z, ec_Z)
        {
          if (cZ->is_complex_cell())   cZ++->demote_complex_to_real(1.0e-15);
          else                         cZ++->demote_float_to_int(1.0e-15);
        }
   }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_B(Value_P B)
{
   return Bif_F12_TRANSPOSE::fun.eval_B(B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_AB(Value_P A, Value_P B)
{
Shape shape_Z;
   loop(r, A->get_rank() - 1)  shape_Z.add_shape_item(A->get_shape_item(r + 1));
   loop(r, B->get_rank() - 1)   shape_Z.add_shape_item(B->get_shape_item(r + 1));

Value_P Z = Bif_F12_RHO::fun.do_reshape(shape_Z, Value::Zero_P)
                            .get_apl_val();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_ROTATE::reverse(Value_P B, Axis axis)
{
   if (B->is_skalar())
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

const Shape3 shape_B3(B->get_shape(), axis);

Value_P Z(new Value(B->get_shape(), LOC));

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         const uint32_t hl = h * shape_B3.m() * shape_B3.l() + l;
         const Cell * cB = &B->get_ravel(hl);
         Cell * cZ = &Z->get_ravel(hl);
         loop(m, shape_B3.m())
            cZ[m*shape_B3.l()].init(cB[(shape_B3.m() - m - 1)*shape_B3.l()]);
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_ROTATE::rotate(Value_P A, Value_P B, Axis axis)
{
const APL_Float qct = Workspace::get_CT();
int32_t gsh = 0;   // global shift (skalar A); 0 means local shift (A) used.

const Shape3 shape_B3(B->get_shape(), axis);
const Shape shape_A2(shape_B3.h(), shape_B3.l());

   if (A->is_skalar_or_len1_vector())
      {
        gsh = A->get_ravel(0).get_near_int(qct);
        if (gsh == 0)   // nothing to do.
           {
             Token result(TOK_APL_VALUE1, B->clone(LOC));
             return result;
           }
      }
   else   // otherwise shape A must be shape B with axis removed.
      {
        A->get_shape().check_same(B->get_shape().without_axis(axis),
                                 E_RANK_ERROR, E_LENGTH_ERROR, LOC);
      }


Value_P Z(new Value(B->get_shape(), LOC));

const Cell * cA = &A->get_ravel(0);

   loop(h, shape_B3.h())
   loop(m, shape_B3.m())
   loop(l, shape_B3.l())
       {
         ShapeItem src = gsh;
         if (!src)   src = A->get_ravel(l + h*shape_B3.l()).get_near_int(qct);
         src += shape_B3.m() + m;
         while (src < 0)               src += shape_B3.m();
         while (src >= shape_B3.m())   src -= shape_B3.m();
         Z->next_ravel()->init(B->get_ravel(shape_B3.hml(h, src, l)));
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_XB(Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());
   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_XB(Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_B(Value_P B)
{
Shape shape_A;

   // monadic transpose: A = ... 4 3 2 1 0
   loop(r, B->get_rank())   shape_A.add_shape_item(B->get_rank() - r - 1);

Value_P Z = transpose(shape_A, B);
   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_AB(Value_P A, Value_P B)
{
   // A should be a skalar or vector.
   //
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_skalar())   // B is a skalar (so A should be empty)
      {
        if (A->element_count() != 0)   LENGTH_ERROR;
        Value_P Z = B->clone(LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape shape_A(A, Workspace::get_CT(), Workspace::get_IO());
   if (shape_A.get_rank() != B->get_rank())   LENGTH_ERROR;

   // the elements in A shall be valid axes of B->
   loop(r, A->get_rank())
      {
        if (shape_A.get_shape_item(r) < 0)               DOMAIN_ERROR;
        if (shape_A.get_shape_item(r) >= B->get_rank())   DOMAIN_ERROR;
      }

Value_P Z = (shape_A.get_rank() == B->get_rank() && is_permutation(shape_A))
          ? transpose(shape_A, B) : transpose_diag(shape_A, B);

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose(const Shape & A, Value_P B)
{
const Shape shape_Z = permute(B->get_shape(), inverse_permutation(A));
Value_P Z(new Value(shape_Z, LOC));

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_values(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b]);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose_diag(const Shape & A, Value_P B)
{
   // check that: ∧/(⍳⌈/0,A)∈A
   //
   // I.e. 0, 1, ... max_A are in A
   // we search sequentially, since A is small.
   // we construct shape_Z as we go.
   //
Shape shape_Z;
   {
     // check that 0 ≤ A[a] ≤ ⍴⍴B, and compute max_A = ⌈/A
     //
     ShapeItem max_A = 0;   // the largest index in A (- qio).
     loop(ra, A.get_rank())
         {
           if (A.get_shape_item(ra) < 0)               DOMAIN_ERROR;
           if (A.get_shape_item(ra) >= B->get_rank())   DOMAIN_ERROR;
           if (max_A < A.get_shape_item(ra))   max_A = A.get_shape_item(ra);
         }

     // check that every m < max_A is in A->
     // the first item found is added shape_Z.
     //
     loop(m, max_A + 1)
         {
           loop(ra, A.get_rank())
               {
                if (m == A.get_shape_item(ra))
                   {
                     shape_Z.add_shape_item(B->get_shape_item(ra));
                     goto found;
                   }
               }

           // not found
           DOMAIN_ERROR;

           found: ;
         }
   }

Value_P Z(new Value(shape_Z, LOC));

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_values(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b]);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::inverse_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                 DOMAIN_ERROR;
         if (rx >= sh.get_rank())    DOMAIN_ERROR;
         if (rho[rx] != -1)          DOMAIN_ERROR;

         rho[rx] = r;
       }

   return Shape(sh.get_rank(), rho);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::permute(const Shape & sh, const Shape & perm)
{
Shape ret;

   loop(r, perm.get_rank())
      {
        ret.add_shape_item(sh.get_shape_item(perm.get_shape_item(r)));
      }

   return ret;
}
//-----------------------------------------------------------------------------
bool
Bif_F12_TRANSPOSE::is_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                return false;
         if (rx >= sh.get_rank())   return false;
         if (rho[rx] != -1)         return false;
          rho[rx] = r;
       }

   return true;
}
//=============================================================================
Token
Bif_F12_DECODE::eval_AB(Value_P A, Value_P B)
{
   // ρZ  is: (¯1↓ρA),1↓ρB
   // ρρZ is: (0⌈¯1+ρρA) + (0⌈¯1+ρρB)
   //
Shape shape_A1;
   if (!A->is_skalar())
      shape_A1 = A->get_shape().without_axis(A->get_rank() - 1);

Shape shape_B1(B->get_shape());
   if (shape_B1.get_rank() > 0)   shape_B1 = B->get_shape().without_axis(0);

const ShapeItem l_len_A = A->get_rank() ? A->get_last_shape_item() : 1;
const ShapeItem h_len_B = B->get_rank() ? B->get_shape_item(0)     : 1;

const ShapeItem h_len_A = shape_A1.get_volume();
const ShapeItem l_len_B = shape_B1.get_volume();

   if (l_len_A == 0 || h_len_B == 0)   // empty result
      {
        const Shape shape_Z(shape_A1 + shape_B1);
        Value_P Z(new Value(shape_Z, LOC));
        new (&Z->get_ravel(0))  IntCell(0);   // prototype
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if ((l_len_A != 1) && (h_len_B != 1) && (l_len_A != h_len_B))   LENGTH_ERROR;

const APL_Float qct = Workspace::get_CT();

const Shape shape_Z = shape_A1 + shape_B1;

Value_P Z(new Value(shape_Z, LOC));

const Cell * cA = &A->get_ravel(0);

   loop(h, h_len_A)
       {
         loop(l, l_len_B)
             {
               const Cell * cB = &B->get_ravel(l);
               decode(Z->next_ravel(), l_len_A, cA, h_len_B, cB, l_len_B, qct);
             }
         cA += l_len_A;
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_DECODE::decode(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB,
                       double qct)
{
APL_Complex value(0.0, 0.0);
APL_Complex weight(1.0, 0.0);
uint32_t len = len_A == 1 ? len_B : len_A;

   cA += len_A - 1;         // point to the lowest item in A
   cB += dB*(len_B - 1);    // point to the lowest item in B
   loop(l, len)
      {
        value += weight*cB[0].get_complex_value();
        weight *= cA[0].get_complex_value();
        if (len_A != 1)   --cA;
        if (len_B != 1)   cB -= dB;
      }

   if (value.imag() > qct)
      new (cZ)   ComplexCell(value);
   else if (value.imag() < -qct)
      new (cZ)   ComplexCell(value);
   else if (Cell::is_near_int(value.real(), qct))
      new (cZ)   IntCell(value.real(), qct);
   else
      new (cZ)   FloatCell(value.real());
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ENCODE::eval_AB(Value_P A, Value_P B)
{
   if (A->is_skalar())   return Bif_F12_STILE::fun.eval_AB(A, B);

const ShapeItem ec_A = A->element_count();
const ShapeItem ec_B = B->element_count();
   if (ec_A == 0 || ec_B == 0)   // empty A or B
      {
        const Shape shape_Z(A->get_shape() + B->get_shape());
        Value_P Z(new Value(shape_Z, LOC));
        new (&Z->get_ravel(0))  IntCell(0);   // prototype
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const ShapeItem ah = A->get_shape_item(0);    // first dimension
const ShapeItem al = A->element_count()/ah;   // remaining dimensions

const APL_Float qct = Workspace::get_CT();

const Shape shape_Z = A->get_shape() + B->get_shape();
const ShapeItem dZ = shape_Z.get_volume()/shape_Z.get_shape_item(0);

Value_P Z(new Value(shape_Z, LOC));

   loop(a1, al)
      {
        const Cell * cB = &B->get_ravel(0);
        loop(b, ec_B)
            {
              encode(dZ, Z->next_ravel(), ah, al, &A->get_ravel(a1), *cB++, qct);
            }
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, const Cell & cB, double qct)
{
   if (!cB.is_numeric())   DOMAIN_ERROR;

APL_Complex value_C = cB.get_complex_value();   // C[] in the standard

   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;
         if (!cA->is_numeric())   DOMAIN_ERROR;

        const ComplexCell cC(value_C);
        cC.bif_residue(cZ, cA);

        if (cA->is_near_zero(qct))
           {
             value_C = APL_Complex(0, 0);
           }
         else
           {
             value_C -= cZ->get_complex_value();
             value_C /= cA->get_complex_value();
           }

         cZ->demote_complex_to_real(qct);
         cZ->demote_float_to_int(qct);
       }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_B(Value_P B)
{
   // enlist
const ShapeItem len_Z = B->get_enlist_count();
Value_P Z(new Value(len_Z, LOC));

Cell * z = &Z->get_ravel(0);
   B->enlist(z, B->is_lval());

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_AB(Value_P A, Value_P B)
{
   // member
   //
const APL_Float qct = Workspace::get_CT();
const ShapeItem len_Z = A->element_count();
const ShapeItem len_B = B->element_count();
Value_P Z(new Value(A->get_shape(), LOC));

   loop(z, len_Z)
       {
         const Cell & cell_A = A->get_ravel(z);
         APL_Integer same = 0;
         loop(b, len_B)
             if (cell_A.equal(B->get_ravel(b), qct))
                {
                  same = 1;
                  break;
                }
         new (&Z->get_ravel(z))   IntCell(same);
       }
   
   Z->set_default(*Value::Zero_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   return partition(A, B, X->get_single_axis(B->get_rank()));
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_B(Value_P B)
{
   if (B->is_simple_skalar())
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

Value_P Z(new Value(LOC));

   new (&Z->get_ravel(0)) PointerCell(B->clone(LOC));   // calls B->set_nested

   Assert(Z->compute_depth() == (B->compute_depth() + 1));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_XB(Value_P X, Value_P B)
{
Shape it_shape;
Shape it_weight;
Shape shape_Z;
Shape weight_Z;

const Shape weight_B = B->get_shape().reverse_scan();

   // put the dimensions mentioned in X into it_shape and the
   // others into shape_Z
   //
   Shape shape_X = X->to_shape();

   loop(r, B->get_rank())        // the axes not in shape_X
       {
         if (!shape_X.contains_axis(r))
            {
              shape_Z.add_shape_item(B->get_shape_item(r));
              weight_Z.add_shape_item(weight_B.get_shape_item(r));
            }
       }

   loop(r, shape_X.get_rank())   // the axes in shape_X
       {
         const ShapeItem x_r = shape_X.get_shape_item(r);

         // check that X∈⍳⍴⍴B
         //
         if (x_r < 0)                AXIS_ERROR;
         if (x_r >= B->get_rank())   AXIS_ERROR;

         it_shape.add_shape_item(B->get_shape_item(x_r));
         it_weight.add_shape_item(weight_B.get_shape_item(x_r));
       }

   if (it_shape.get_rank() == 0)   // empty axes
      {
        //  ⊂[⍳0]B   ←→   ⊂¨B
        Token part(TOK_FUN1, &Bif_F12_PARTITION::fun);
        return Bif_OPER1_EACH::fun.eval_LB(part, B);
      }

Value_P Z(new Value(shape_Z, LOC));

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
      {
        const ShapeItem off_Z = it_Z.multiply(weight_Z);   // offset in B

        Value_P vZ(new Value(it_shape, LOC));
        new (Z->next_ravel()) PointerCell(vZ);

        Cell * dst_it = &vZ->get_ravel(0);
        for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
           {
             const ShapeItem off_it = it_it.multiply(it_weight);  // offset in B
             dst_it++->init(B->get_ravel(off_Z + off_it));
           }

        vZ->check_value(LOC);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::partition(Value_P A, Value_P B, Axis axis)
{
   if (A->get_rank() > 1)    RANK_ERROR;
   if (B->get_rank() == 0)   RANK_ERROR;

const APL_Float qct = Workspace::get_CT();
   if (A->is_skalar())
      {
        APL_Integer val = A->get_ravel(0).get_int_value();
        if (val == 0)
           {
             return Token(TOK_APL_VALUE1, Value::Idx0_P);
           }

        return eval_B(B);
      }

   if (A->get_shape_item(0) != B->get_shape_item(axis))   LENGTH_ERROR;

   // determine the length of the partitioned dimension...
   //
ShapeItem len_Z = 0;
   {
     ShapeItem prev = 0;
     loop(l, A->get_shape_item(0))
        {
           const APL_Integer am = A->get_ravel(l).get_int_value();
           if (am < 0)   DOMAIN_ERROR;
           if (am  > prev)   ++len_Z;
           prev = am;
        }
   }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, len_Z);

Value_P Z(new Value(shape_Z, LOC));


   if (Z->is_empty())
      {
        Z->set_default(*B.get());

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_B3(B->get_shape(), axis);
Cell * cZ;

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         ShapeItem from = -1;
         ShapeItem prev_am = 0;
         ShapeItem zm = 0;
         cZ = &Z->get_ravel(h*len_Z*shape_B3.l() + l);

         loop(m, shape_B3.m())
             {
               const APL_Integer am = A->get_ravel(m).get_int_value();
               Assert(am >= 0);   // already verified above.

               if (am == 0)   // skip element m (and following)
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, h, from, m, shape_B3.m(),
                                      l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = -1;
                  }
               else if (am > prev_am)   // new segment
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, h, from, m,
                                      shape_B3.m(), l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = m;
                  }
               prev_am = am;
             }

         if (from != -1)   // an old segment is pending
            {
              copy_segment(cZ + zm, h, from, shape_B3.m(),
                           shape_B3.m(), l, shape_B3.l(), B);

              // zm += shape_B3.l();   not needed since we are done.
            }
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_PARTITION::copy_segment(Cell * dest, ShapeItem h, ShapeItem m_from,
                                ShapeItem m_to, ShapeItem m_len,
                                ShapeItem l, ShapeItem l_len, Value_P B)
{
   Assert(m_from < m_to);
   Assert(m_to <= m_len);
   Assert(l < l_len);

Value_P V(new Value(m_to - m_from, LOC));

Cell * vv = &V->get_ravel(0);
   for (ShapeItem m = m_from; m < m_to; ++m)
       {
         const Cell & cb = B->get_ravel(l + (m + h*m_len)*l_len);
         vv++->init(cb);
       }

   new (dest) PointerCell(V);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::eval_B(Value_P B)
{
const ShapeItem len_B = B->element_count();
const Shape it_shape = item_shape(B);;
const Shape shape_Z = B->get_shape() + it_shape;

Value_P Z(new Value(shape_Z, LOC));

const ShapeItem llen = it_shape.get_volume();
   loop(h, len_B)
       {
         const Cell & B_item = B->get_ravel(h);
         if (B_item.is_pointer_cell())
            {
              Value_P vB = B_item.get_pointer_value();
              Bif_F12_TAKE::fill(it_shape, &Z->get_ravel(h*llen), vB);
            }
         else if (B_item.is_character_cell())   // simple char skalar
            {
              Z->get_ravel(h*llen).init(B_item);
              for (ShapeItem c = 1; c < llen; ++c)
                  Z->get_ravel(h*llen + c).init(c_filler);
            }
         else   // simple skalar
            {
              Z->get_ravel(h*llen).init(B_item);
              for (ShapeItem c = 1; c < llen; ++c)
                  Z->get_ravel(h*llen + c).init(n_filler);
            }
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::eval_XB(Value_P X, Value_P B)
{
   // disclose with axis

   // all items of B must have the same rank. it_shape is the smallest
   // shape that can contain each item of B.
   //
const Shape it_shape = item_shape(B);

   // init axes_X with the axes in the ravel of X, normalized to ⎕IO = 0
   //
const Shape axes_X = X->to_shape();

   // the number of items in X must be the number of axes in it_shape
   if (it_shape.get_rank() != axes_X.get_rank())   AXIS_ERROR;

   // distribute shape_B and it_shape into 2 shapes perm and shape_Z.
   // if a dimension is mentioned in axes_X then it goes into shape_Z and
   // otherwise into perm.
   //
Shape perm;
Shape shape_Z;
   {
     ShapeItem B_idx = 0;
     //
     loop(z, B->get_rank() + it_shape.get_rank())
        {
          // check if z is in X, remembering its position if so.
          //
          bool z_in_X = false;
          ShapeItem x_pos = -1;
          loop(x, axes_X.get_rank())
             if (axes_X.get_shape_item(x) == z)
                {
                  z_in_X = true;
                  x_pos = x;
                  break;
                }

          if (z_in_X)   // z is an item dimension: put it in shape_Z
             {
               shape_Z.add_shape_item(it_shape.get_shape_item(x_pos));
             }
          else          // z is a B dimension: put it in perm
             {
               if (B_idx >= B->get_rank())   INDEX_ERROR;
               perm.add_shape_item(z);
               shape_Z.add_shape_item(B->get_shape_item(B_idx++));
             }
        }

     // append X to perm with each X item reduced by the B items before it.
     loop(x, axes_X.get_rank())
        {
          Rank before_x = 0;   // items before X that are not in X
          loop(x1, x - 1)
             if (!axes_X.contains_axis(x1))   ++before_x;

          perm.add_shape_item(axes_X.get_shape_item(x) + before_x);
        }
   }

Value_P Z(new Value(shape_Z, LOC));

   // loop over sources and place them in the result.
   //
PermutedArrayIterator it_Z(shape_Z, perm);

   for (ArrayIterator it_B(B->get_shape()); !it_B.done(); ++it_B)
      {
        const Cell & B_item = B->get_ravel(it_B.get_total());
        const Cell * src = 0;
        if (B_item.is_pointer_cell())
           {
             Value_P vB = B_item.get_pointer_value();
             ArrayIterator vB_it(vB->get_shape());
             for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
                 {
                   if (vB->get_shape().contains(it_it.get_values()))
                      {
                        src = &vB->get_ravel(vB_it.get_total());
                        ++vB_it;
                      }
                   else if (vB->get_ravel(0).is_character_cell())  // char
                        src = &c_filler;
                   else                                   // simple numeric
                        src = &n_filler;

                   Z->get_ravel(it_Z.get_total()).init(*src);
                   ++it_Z;
                 }
           }
        else
           {
             for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
                 {
                   if (it_it.get_total() == 0)   // first element: use B_item
                      {
                        src = &B_item;
                      }
                   else if (B_item.is_character_cell())   // simple char skalar
                      {
                        src = &c_filler;
                      }
                   else                                // simple numeric skalar
                      {
                        src = &n_filler;
                      }

                   Z->get_ravel(it_Z.get_total()).init(*src);
                   ++it_Z;
                 }
            }
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_PICK::item_shape(Value_P B)
{
   // all items are skalars or arrays of the same rank R.
   // return the shape with rank R and the (per-dimension) max. of
   // each shape item
   //
const ShapeItem len_B = B->element_count();

Shape ret;   // of the first non-skalar in B

   loop(b, len_B)
       {
         const Cell & cell = B->get_ravel(b);
         if (!cell.is_pointer_cell())   continue;   // simple skalar

         Value_P v = cell.get_pointer_value();

         if (ret.get_rank() == 0)   // first non-skalar
            {
              ret = v->get_shape();
              continue;
            }

         // the items of B must have the same rank.
         //
         if (ret.get_rank() != v->get_rank())   RANK_ERROR;

         loop(r, ret.get_rank())
             {
               if (ret.get_shape_item(r) < v->get_shape_item(r))
                  {
                    ret.set_shape_item(r, v->get_shape_item(r));
                  }
             }
       }

   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)    RANK_ERROR;

const ShapeItem ec_A = A->element_count();

   // if A is empty, return B
   //
   if (ec_A == 0)   return Token(TOK_APL_VALUE1, B);

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

Value_P Z = pick(&A->get_ravel(0), ec_A, B, qct, qio, B->is_left());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PICK::pick(const Cell * cA, ShapeItem len_A, Value_P B,
                   APL_Float qct, APL_Integer qio, bool lval)
{
ShapeItem c = 0;

   if (cA->is_pointer_cell())   // B is an array
      {
        Value_P A = cA->get_pointer_value();
        if (A->get_rank() > 1)   RANK_ERROR;

        const ShapeItem len_A = A->element_count();
        if (B->get_rank() != len_A)   RANK_ERROR;

        const Shape weight = B->get_shape().reverse_scan();
        const Shape A_as_shape(A, qct, qio);

        loop(r, A->element_count())
            c += weight.get_shape_item(r) * A_as_shape.get_shape_item(r);
      }
   else   // A is a skalar, so B must be a vector.
      {
        if (B->get_rank() != 1)         RANK_ERROR;
        const APL_Integer a = cA->get_near_int(qct) - qio;
        if (a < 0)                      DOMAIN_ERROR;
        if (a > B->get_shape_item(0))   LENGTH_ERROR;
        c = a;
      }

const Cell * cB = &B->get_ravel(c);

   if (len_A > 1)   // more levels coming.
      {
        if (cB->is_pointer_cell())
           {
             return pick(cA + 1, len_A - 1, cB->get_pointer_value(),
                         qct, qio, lval);
           }

        if (cB->is_lval_cell())
           {
             Assert(lval);
             Cell & cell = *cB->get_lval_value();
             if (!cell.is_pointer_cell())   DOMAIN_ERROR;

             Value_P subval = cell.get_pointer_value();
             Value_P subrefs = subval->get_cellrefs(LOC);
             return pick(cA + 1, len_A - 1, subrefs, qct, qio, lval);
           }

        // at this point the depth implied by A is greater than the
        // depth of B.
        //
        DOMAIN_ERROR;
      }

   // at this point, cB is the cell in B pick'ed by A->
   //
   if (cB->is_pointer_cell())
      {
        Value_P Z = cB->get_pointer_value()->clone(LOC);
        return Z;
      }

   if (cB->is_lval_cell())   // e.g. (A⊃B) ← C
      {
        Assert(lval);

        Cell * cell = cB->get_lval_value();
        if (cell->is_pointer_cell())
           {
             Value_P sub = cell->get_pointer_value();
             return sub->get_cellrefs(LOC);
           }

        Value_P Z(new Value(LOC));
        new (Z->next_ravel())   LvalCell(cell);
        return Z;
      }
   else if (lval)   // e.g. (A⊃C) ← B
      {
        Value_P Z(new Value(LOC));
        new (Z->next_ravel()) LvalCell((Cell *)cB);
        return Z;
      }
   else
      {
        Value_P Z(new Value(LOC));
        Z->next_ravel()->init(*cB);
        return Z;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_A = A->element_count();
   if (ec_A != B->get_rank())   RANK_ERROR;

   // index_expr is in reverse order!
   //
IndexExpr * index_expr = new IndexExpr(false, LOC);
   loop(a, ec_A)
      {
         const Cell & cell = A->get_ravel(ec_A - a - 1);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_int_value();
              val = Value_P(new Value(LOC));
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }
        index_expr->add(val);
      }

   index_expr->quad_ct = Workspace::get_CT();
   index_expr->quad_io = Workspace::get_IO();

Value_P Z;
   if (index_expr->value_count() == 1)   // one-dimensional index
      {
        Value_P single_index = index_expr->extract_value(0);
        Z = B->index(single_index);
      }
   else                                  // 0- or multi-dimensional index
      {
        Z = B->index(*index_expr);
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const Shape axes_present = X->to_shape();

const ShapeItem ec_A = A->element_count();
   if (ec_A != axes_present.get_rank())   RANK_ERROR;
   if (ec_A > B->get_rank())              RANK_ERROR;

   // index_expr is in reverse order!
   //
const APL_Integer qio = Workspace::get_IO();
IndexExpr * index_expr = new IndexExpr(false, LOC);
   loop(rb, B->get_rank())   index_expr->add(Value_P());
   index_expr->quad_ct = Workspace::get_CT();
   index_expr->quad_io = qio;

   loop(a, ec_A)
      {
         const Rank axis = axes_present.get_shape_item(a);

         const Cell & cell = A->get_ravel(a);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_int_value();
              val = Value_P(new Value(LOC));
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }

        index_expr->set_value(axis, val);
      }

Value_P Z = B->index(*index_expr);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_B(Value_P B)
{
Value_P Z;
const Cell & first_B = B->get_ravel(0);

   if (B->element_count() == 0)   // return prototype
      {
        Z = B->prototype(LOC);
      }
   else if (first_B.is_pointer_cell())
      {
        Value_P v1 = first_B.get_pointer_value();
        if (v1->is_lval())
           {
             Value_P B1(new Value(LOC));
             new (&B1->get_ravel(0))   PointerCell(v1);

             Z = Value_P(new Value(LOC));
             new (&Z->get_ravel(0))   LvalCell(&B1->get_ravel(0));
           }
        else
           {
             const ShapeItem ec = v1->nz_element_count();
             Z = Value_P(new Value(v1->get_shape(), LOC));
             loop(e, ec)   Z->get_ravel(e).init(v1->get_ravel(e));
           }
      }
   else
      {
        Z = Value_P(new Value(LOC));

        Z->get_ravel(0).init(first_B);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (X->element_count() == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   // A↑[X]B ←→ ⊃[X](⊂A)↑¨⊂[X]B
   //
Value_P cA = Bif_F12_PARTITION::fun.eval_B(A).get_apl_val();           // ⊂A
Value_P cB = Bif_F12_PARTITION::fun.eval_XB(X, B).get_apl_val();       // ⊂[X]B
Token take(TOK_FUN2, &Bif_F12_TAKE::fun);
Value_P cT = Bif_OPER1_EACH::fun.eval_ALB(cA, take, cB).get_apl_val(); // cA↑¨cB

Token result = Bif_F12_PICK::fun.eval_XB(X, cT);
   return result;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AB(Value_P A, Value_P B)
{
   // A must be an integer skalar or vector
   //
   if (A->get_rank() > 1)   RANK_ERROR;

Shape ravel_A(A, Workspace::get_CT(), 0);
Shape ravel_A1(ravel_A);
   if (ravel_A1.get_rank() == 0)   ravel_A1.add_shape_item(1);   // A ← ,A
   if (ravel_A1.get_rank() > MAX_RANK)     LENGTH_ERROR;

   if (B->is_skalar())
      {
        if (ravel_A.get_rank() == 0)
           {
             Token result(TOK_APL_VALUE1, B->clone(LOC));
             return result;
           }

        Cell proto;
        proto.init_type(B->get_ravel(0));
        const Shape ravel_A1_abs = ravel_A1.abs();
        Value_P Z(new Value(ravel_A1_abs, LOC));
        const ShapeItem ec_Z = Z->nz_element_count();   // incl. proto

        loop(z, ec_Z)   Z->next_ravel()->init(proto);

        // compute the position of the non-default element. It is the first
        // item of the corresponfig dimension if the axis is positive, or the
        // last element if the axis is negative.
        //
        ShapeItem pos = 0;
        ShapeItem weight = 1;
        loop(r, ravel_A1.get_rank())
            {
              const ShapeItem len = ravel_A1.get_shape_item(r);
              if (len < 0)   { pos -= (1 + len) * weight;   weight *= -len; }
              else           {                              weight *=  len; }
            }
        Z->get_ravel(pos).init(B->get_ravel(0));

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (ravel_A1.get_rank() != B->get_rank())   RANK_ERROR;

   return do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::do_take(const Shape shape_Zi, Value_P B)
{
Shape shape_Z(shape_Zi);

   loop(r, shape_Zi.get_rank())
       {
         ShapeItem a = shape_Zi.get_shape_item(r);

         if (a < 0)    shape_Z.set_shape_item(r, -a);
       }

Value_P Z(new Value(shape_Z, LOC));

   fill(shape_Zi, &Z->get_ravel(0), B);

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_TAKE::fill(const Shape & shape_Zi, Cell * Z, Value_P B)
{
   if (shape_Zi.is_empty())   return;

   Assert(shape_Zi.get_rank() == B->get_rank());

   // compute shape_Z = abs(shape_Zi), and Q with Z[i] = B[Q + i].
   //
Shape shape_Z;
Shape Q;

   loop(r, shape_Zi.get_rank())
       {
         const ShapeItem Zi = shape_Zi.get_shape_item(r);

         if (Zi >= 0)
            {
              shape_Z.add_shape_item(Zi);
              Q.add_shape_item(0);
            }
         else
            {
              shape_Z.add_shape_item(-Zi);
              Q.add_shape_item(B->get_shape_item(r) + Zi);
            }
       }

const Shape weight_B = B->get_shape().reverse_scan();

   for (ArrayIterator it(shape_Z); !it.done(); ++it)
      {
        ShapeItem bpos = 0;
        bool fill = false;
        loop(r, shape_Z.get_rank())
            {
              const ShapeItem idx_B = it.get_value(r) + Q.get_shape_item(r);
              if ((idx_B <  0) || (idx_B >= B->get_shape_item(r)))
                 {
                   fill = true;
                   break;
                 }

              bpos += weight_B.get_shape_item(r) * idx_B;
            }

        if (fill)   Z->init_type(B->get_ravel(0));
        else        Z->init(B->get_ravel(bpos));
        ++Z;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DROP::eval_AB(Value_P A, Value_P B)
{
Shape ravel_A(A, Workspace::get_CT(), 0);
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_skalar())
      {
        // if B is a skalar then the result rank shall be the length of A->
        // the result may be empty (shape 0 0 ... 0) if we drop something
        // or non-empty (shape 1 1 ... 1) if we drop nothing.
        //
        const ShapeItem len_Z = ravel_A.get_volume() ? 0 : 1;

        Shape shape_Z;
        loop(r, ravel_A.get_rank())   shape_Z.add_shape_item(len_Z);

        Value_P Z(new Value(shape_Z, LOC));

        Z->get_ravel(0).init(B->get_ravel(0));
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (ravel_A.get_rank() == 0)   ravel_A.add_shape_item(1);   // A = ,A

   if (ravel_A.get_rank() != B->get_rank())   RANK_ERROR;

   loop(r, ravel_A.get_rank())
       {
         const APL_Integer a = ravel_A.get_shape_item(r);
         const ShapeItem amax = B->get_shape_item(r);

         if      (a >= amax)  ravel_A.set_shape_item(r, 0);
         else if (a >= 0)     ravel_A.set_shape_item(r, a - amax);
         else if (a > -amax)  ravel_A.set_shape_item(r, amax + a);
         else                 ravel_A.set_shape_item(r, 0);
       }

   return Bif_F12_TAKE::do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DROP::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (X->element_count() == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   if (X->get_rank() > 1)    INDEX_ERROR;

const uint64_t len_X = X->element_count();
   if (len_X > MAX_RANK)     INDEX_ERROR;
   if (len_X == 0)           INDEX_ERROR;

   if (A->get_rank() > 1)    RANK_ERROR;

uint64_t len_A = A->element_count();
   if (len_A != len_X)   LENGTH_ERROR;

const APL_Float   qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   // init ravel_A = shape_B and seen.
   //
Shape ravel_A(B->get_shape());
bool seen[MAX_RANK];
   loop(r, B->get_rank())   seen[r] = false;

   loop(r, len_X)
       {
         const APL_Integer a = A->get_ravel(r).get_near_int(qct);
         const uint32_t x = X->get_ravel(r).get_near_int(qct) - qio;

         if (x >= B->get_rank())   INDEX_ERROR;
         if (seen[x])              INDEX_ERROR;
         seen[x] = true;

         const int32_t amax = B->get_shape_item(x);
         if      (a >= amax)   ravel_A.set_shape_item(x, 0);
         else if (a >= 0)      ravel_A.set_shape_item(x, a - amax);
         else if (a > -amax)   ravel_A.set_shape_item(x, amax + a);
         else                  ravel_A.set_shape_item(x, 0);
       }

   return Bif_F12_TAKE::do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_SORT::sort(Value_P B, bool ascending)
{
   if (B->is_skalar())   RANK_ERROR;

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)
      {
        Value_P Z = Value::Idx0_P;
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const APL_Integer qio = Workspace::get_IO();
const ShapeItem comp_len = B->element_count()/len_BZ;
   loop(b, len_BZ)
       if (B->get_ravel(b).is_pointer_cell())
          DOMAIN_ERROR;

const Cell * array[len_BZ];

   loop(bz, len_BZ)   array[bz] = &B->get_ravel(bz*comp_len);

   Cell::heapsort(array, len_BZ, ascending, &comp_len, &Cell::greater_vec);

Value_P Z(new Value(len_BZ, LOC));

const Cell * base = &B->get_ravel(0);
   loop(bz, len_BZ)
       new (&Z->get_ravel(bz)) IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default(*Value::Zero_P);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_SORT::sort_collating(Value_P A, Value_P B, bool ascending)
{
   if (A->is_skalar())   RANK_ERROR;
   if (A->NOTCHAR())     DOMAIN_ERROR;

const APL_Integer qio = Workspace::get_IO();
   if (B->NOTCHAR())     DOMAIN_ERROR;
   if (B->is_skalar())   return Token(TOK_APL_VALUE1, qio ? Value::One_P
                                                          : Value::Zero_P);

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)   return Token(TOK_APL_VALUE1, Value::Idx0_P);
   if (len_BZ == 1)   return Token(TOK_APL_VALUE1, qio ? Value::One_P
                                                       : Value::Zero_P);

Value_P B1(new Value(B->get_shape(), LOC));
const ShapeItem ec_B = B->element_count();
const ShapeItem comp_len = ec_B/len_BZ;
CollatingCache cc_cache(A->get_rank(), comp_len);
   loop(b, ec_B)
      {
        const Unicode uni = B->get_ravel(b).get_char_value();
        const ShapeItem b1 = collating_cache(uni, A, cc_cache);
        new (&B1->get_ravel(b)) IntCell(b1);
      }

const Cell * array[len_BZ];
   loop(bz, len_BZ)   array[bz] = &B1->get_ravel(bz*comp_len);

   Cell::heapsort(array, len_BZ, ascending, &cc_cache,
                  &CollatingCache::greater_vec);

Value_P Z(new Value(len_BZ, LOC));

const Cell * base = &B1->get_ravel(0);
   loop(bz, len_BZ)
       new (&Z->get_ravel(bz)) IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default(*Value::Zero_P);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
ShapeItem
Bif_SORT::collating_cache(Unicode uni, Value_P A, CollatingCache & cache)
{
   // first check if uni is already in the cache...
   //
   loop(s, cache.size())
      {
        if (uni == cache[s].ce_char)   return s;
      }

   // uni is not in the cache. See if it is in the collating sequence.
   //
bool uni_in_A = false;
const ShapeItem ec_A = A->element_count();
CollatingCacheEntry entry(uni, A->get_shape());
   loop(a, ec_A)
      {
        if (uni != A->get_ravel(a).get_char_value())   continue;

        uni_in_A = true;
        ShapeItem aq = a;
        loop(r, entry.ce_shape.get_rank())
           {
             const Rank axis = entry.ce_shape.get_rank() - r - 1;
             const ShapeItem ar = aq % A->get_shape_item(axis);
             if (entry.ce_shape.get_shape_item(axis) > ar)
                entry.ce_shape.set_shape_item(axis, ar);
             aq /= A->get_shape_item(axis);
           }
      }

   cache.push_back(entry);
   return cache.size() - 1;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_EQUIV::eval_B(Value_P B)
{
const Depth depth = B->compute_depth();

Value_P Z(new Value(LOC));

   new (&Z->get_ravel(0)) IntCell(depth);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_EQUIV::eval_AB(Value_P A, Value_P B)
{
   // match
   //

const APL_Float qct = Workspace::get_CT();
const ShapeItem count = A->nz_element_count();  // compare at least the prototype
bool different = false;

   if (!A->same_shape(B))
      {
        different = true;
        goto done;
      }

   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
          {
            different = true;
            break;
          }
done:

Value_P Z = different ? Value::Zero_P : Value::One_P;
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_FORMAT::eval_B(Value_P B)
{
Value_P Z = monadic_format(B);
   Z->set_default(*Value::Spc_P);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_FORMAT::eval_AB(Value_P A, Value_P B)
{
Value_P Z;

   if (A->is_char_vector() || A->is_char_skalar())
      {
        Z = format_by_example(A, B);
      }
   else if (A->is_int_vector(Workspace::get_CT())
              || A->is_int_skalar(Workspace::get_CT()))
      {
        Z = format_by_specification(A, B);
      }
   else
      {
        A->print_boxed(CERR, "left argument of ⍕: ");
        DOMAIN_ERROR;
      }

   Z->set_default(*Value::Spc_P);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::monadic_format(Value_P B)
{
PrintContext pctx;
   pctx.set_style(PrintStyle(pctx.get_style() | PST_NO_FRACT_0));
const PrintBuffer pb(*B, pctx);

const uint32_t width  = pb.get_width(0);
const uint32_t height = pb.get_height();

Value_P Z;
   if (height == 1)   Z = Value_P(new Value(width, LOC));
   else               Z = Value_P(new Value(Shape(height, width), LOC));

   loop(h, height)
   loop(w, width)
      {
        const Unicode uni = pb.get_char(w, h);
        if (is_pad_char(uni))   new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
        else                    new (Z->next_ravel()) CharCell(uni);
      }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::format_by_example(Value_P A, Value_P B)
{
   if (A->get_rank() != 1)   RANK_ERROR;

   // convert the ravel of char vector A into an UCS_string.
   //
UCS_string format = A->get_UCS_ravel();
   if (format.size() == 0)   LENGTH_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();

   // split format into format fields, one per column.
   // If there is only one format field, then repeat it cols times.
   //
vector<UCS_string> col_formats;
   split_example_into_columns(format, col_formats);
   if (col_formats. size() == 1)
      {
        UCS_string f = format;
        loop(c, cols - 1)
           {
             format.append(f);
             col_formats.push_back(col_formats.front());
           }
      }

   if (col_formats.size() != cols)   LENGTH_ERROR;

   // convert each column format string into a Format_LIFER
   //
vector<Format_LIFER> col_items;
   loop(c, col_formats.size())
       col_items.push_back(Format_LIFER(col_formats[c]));

   Log(LOG_Bif_F12_FORMAT)
      {
        Q(format)
        Q(format.size())
        Q(col_formats.size())

        loop(c, col_items.size())
            {
              Q(col_formats[c])

              Q(col_items[c].left_deco)
              Q(col_items[c].int_part)
              Q(col_items[c].fract_part)
              Q(col_items[c].expo_deco)
              Q(col_items[c].exponent)
              Q(col_items[c].expo_negative)
              Q(col_items[c].right_deco)
            }
      }

Shape shape_Z(B->get_shape());
   if (B->is_skalar())   shape_Z.add_shape_item(1);
   shape_Z.set_last_shape_item(format.size());

Value_P Z(new Value(shape_Z, LOC));

   try
      {
        loop(r, rows)
           {
             UCS_string row;
             loop(c, cols)
                {
                  const Cell & cB = B->get_ravel(c + r*cols);
                  if (!cB.is_real_cell())   DOMAIN_ERROR;

                  const APL_Float value = cB.get_real_value();

                  const UCS_string item = col_items[c].format_example(value);
                  Log(LOG_Bif_F12_FORMAT)   Q(item)
                  row.append(item);
                }

             Log(LOG_Bif_F12_FORMAT)
                { Q(row) Q(format) }

             Assert(row.size() == format.size());
             loop(c, row.size())
                new (&Z->get_ravel(r*format.size() + c))  CharCell(row[c]);
           }
      }
   catch (Error err)
      {
        throw err;   // rethrow
      }
   catch (...)
      {
      }

   return Z;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::split_example_into_columns(const UCS_string & format,
                                   vector<UCS_string> & col_formats)
{
bool fmt_seen = false;
UCS_string fmt;
   loop(f, format.size())
       {
         const Unicode cc = format[f];
         fmt.append(cc);

         const bool is_fmt_char = is_control_char(cc);
         if (is_fmt_char)   fmt_seen = true;

         if ((cc == UNI_ASCII_SPACE) && fmt_seen)   // end of field
            {
              col_formats.push_back(fmt);
              fmt.clear();    // start a new field;
              fmt_seen = false;
              continue;   // next char
            }

         if (cc == UNI_ASCII_6)   // end of field after next char
            {
              ++f;   // next char is right decorator (and end of field)
              if (f < format.size())   fmt.append(format[f]);

              col_formats.push_back(fmt);
              fmt.clear();    // start a new field;
              fmt_seen = false;
              continue;   // next char
            }
       }

   if ((!fmt_seen) && (col_formats.size() > 0))
      {
        col_formats.back().append(fmt);
      }
   else
      {
        col_formats.push_back(fmt);
      }
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_example(APL_Float value)
{
UCS_string data_int;
UCS_string data_fract;
UCS_string data_expo;
const double val_1 = (value < 0) ? -value : value;

   fill_data_fields(val_1, data_int, data_fract, data_expo);
   Log(LOG_Bif_F12_FORMAT)
      {
        Q(data_int)
        Q(data_fract)
        Q(data_expo)
      }

bool overflow = false;
const UCS_string left = format_left_side(data_int, value < 0, overflow);
   if (overflow)   return UCS_string(out_size(), Workspace::get_FC(3));
   Assert(left.size() == (left_deco.out_len + int_part.out_len));

const UCS_string right = format_right_side(data_fract, value<0, data_expo);
   Assert(right.size() == (fract_part.out_len + expo_deco.out_len +
                           exponent.out_len + right_deco.out_len));

   return left + right;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_left_side(const UCS_string data_int,
                                               bool negative, bool & overflow)
{
const UCS_string data = int_part.insert_int_commas(data_int, overflow);

   if (overflow)   return UCS_string();

   Assert(int_part.size() >= data.size());
const Unicode pad_char = int_part.pad_char(Workspace::get_FC(2));
const UCS_string pad(int_part.size() - data.size(), pad_char);

UCS_string ucs;
   if (int_part.no_float())                 // floating disabled.
      {
        ucs.append(left_deco.format);
        ucs.append(pad);
      }
   else if (int_part.do_float(negative))   // floating enabled, deco shown
      {
        ucs.append(pad);
        ucs.append(left_deco.format);
      }
   else                                     // floating enabled, deco hidden
      {
        ucs.append(pad);
        ucs.append(UCS_string(left_deco.size(), UNI_ASCII_SPACE));
      }

   ucs.append(data);
   return ucs;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_right_side(const UCS_string data_fract,
                                                bool negative,
                                                const UCS_string data_expo)
{
int pad_count = 0;
UCS_string ucs;

   if (fract_part.out_len)
      {
        const UCS_string data = fract_part.insert_fract_commas(data_fract);
        Assert(fract_part.size() >= data.size());
        pad_count += fract_part.size() - data.size();

        // print nothing instead of .
        if (data.size() == 0)   ++pad_count;
        else                    ucs.append(Workspace::get_FC(0)); 
        ucs.append(data);
      }

   if (exponent.size())
      {
        UCS_string data(1, exponent_char);

        const Unicode pad_char = exponent.pad_char(Workspace::get_FC(2));

        if (exponent.no_float())              // floating disabled.
           {
             data.append(expo_deco.format);
             data.append(data_expo);
           }
        else if (exponent.do_float(expo_negative))   // floating, deco shown
           {
             data.append(expo_deco.format);
             data.append(data_expo);
           }
        else                                 // floating enabled and deco hidden
           {
             data.append(UCS_string(expo_deco.format.size(), pad_char));
             data.append(data_expo);
           }

        pad_count += expo_deco.out_len + exponent.out_len - data.size();
        ucs.append(data);
      }

   // now do the right side padding.
   {
     const Unicode pad_char = fract_part.pad_char(Workspace::get_FC(2));
     const UCS_string pad(pad_count, pad_char);

     if (fract_part.no_float())                // floating disabled.
        {
          ucs.append(right_deco.format);
          ucs.append(pad);
        }
     else if (fract_part.do_float(negative))  // floating, deco shown
        {
          ucs.append(pad);
          ucs.append(right_deco.format);
        }
     else                                      // floating, deco hidden
        {
          ucs.append(pad);
          ucs.append(UCS_string(right_deco.size(), pad_char));
        }
   }

   return ucs;
}
//-----------------------------------------------------------------------------
Bif_F12_FORMAT::Format_LIFER::Format_LIFER(const UCS_string format)
   : exponent_char(UNI_ASCII_E),
     expo_negative(false)
{
   // we split format into our format chunks...
   //
size_t f = 0;
bool exponent_pending = false;

left_decorator:
   while (f < format.size())
      {
        if (is_control_char(format[f]))   goto integral_part;
        else                              left_deco.format.append(format[f++]);
      }
   goto fields_done;

integral_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (cc == UNI_ASCII_FULLSTOP)   goto fractional_part;
        if (cc == UNI_ASCII_E)          goto exponent_part;
        if (is_control_char(cc))
           {
             int_part.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             --f;
            goto right_decorator;
           }
      }
   goto fields_done;

fractional_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];
        if (cc == UNI_ASCII_7)          exponent_pending = true;

        if (is_control_char(cc))
           {
             fract_part.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             exponent_char = cc;
             goto exponent_decorator;
           }
      }
   goto fields_done;

exponent_decorator:
   if (!exponent_pending)   { --f;   goto right_decorator; }

   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (!is_control_char(cc))
           {
             expo_deco.format.append(cc);
           }
        else
           {
             --f;
             goto exponent_part;
           }
      }

exponent_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (is_control_char(cc))
           {
             exponent.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             --f;
             goto right_decorator;
           }
      }

right_decorator:   /// the right decorator
   while (f < format.size())
      right_deco.format.append(format[f++]);

fields_done:

int sum = 0;
   sum += left_deco.out_len = left_deco .format.size();

   sum += int_part  .out_len = int_part  .format.size();

   if (fract_part.format.size())
      sum += fract_part.out_len = fract_part.format.size() + 1;

   if (exponent.size())
      {
        sum += expo_deco.out_len = expo_deco.size();
        sum += exponent.out_len = exponent.format.size() + 1;
      }

   right_deco.out_len = right_deco.format.size();

   // compute floating flags.
   //
   {
     const int count = int_part  .map_field(0)
                     + fract_part.map_field(1);

     if (count == 1)   // flags apply to both sides.
        {
          int_part.  flt_mask |= fract_part.flt_mask & (BIT_1 | BIT_2 | BIT_3);
          fract_part.flt_mask |= int_part  .flt_mask & (BIT_1 | BIT_2 | BIT_3);
        }

     exponent.map_field(2);
   }
}
//-----------------------------------------------------------------------------
int
Format_sub::map_field(int type)
{
int flt_cnt = 0;

   loop(f, format.size())
      {
        switch(format[f])
           {
             case UNI_ASCII_0:              flt_mask |= BIT_0;   break; 
             case UNI_ASCII_1: ++flt_cnt;   flt_mask |= BIT_1;   break; 
             case UNI_ASCII_2: ++flt_cnt;   flt_mask |= BIT_2;   break; 
             case UNI_ASCII_3: ++flt_cnt;   flt_mask |= BIT_3;   break; 
             case UNI_ASCII_4: ++flt_cnt;   flt_mask |= BIT_4;   break; 
             case UNI_ASCII_5:              flt_mask |= BIT_5;   break; 
             case UNI_ASCII_6:              flt_mask |= BIT_6;   break; 
             case UNI_ASCII_7:              flt_mask |= BIT_7;   break; 
             case UNI_ASCII_8:              flt_mask |= BIT_8;   break; 
             case UNI_ASCII_9:              flt_mask |= BIT_9;   break;
           }
      }

   if (flt_mask & (BIT_0 | BIT_9))
      {
        min_len = format.size();
        loop(f, format.size())
            {
              const size_t pos = (type == 1) ? format.size() - f - 1 : f;
              const Unicode uni = format[pos];
              if (uni == UNI_ASCII_0)   break;
              if (uni == UNI_ASCII_9)   break;
              --min_len;
            }
      }

   return flt_cnt;
}
//-----------------------------------------------------------------------------
ostream &
Format_sub::print(ostream & out) const
{
   out << "format '" << format << "'"
          " min " << min_len << ", out_len " << out_len << ", flags: ";
   loop(d, 32)   if (flt_mask & (1 << d))   out << char('0' + d);
   return out;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::Format_LIFER::fill_data_fields(double value,
                UCS_string & data_int, UCS_string & data_fract,
                UCS_string & data_expo)
{
char format[40];
char data_buf[int_part.out_len + 1 + fract_part.out_len + 1 + exponent.out_len + 1];
char * fract_end = 0;

   if (exponent.size())
      {
        // create a format like %.5E (if fract_part has 5 digits)
        //
        const int flen = snprintf(format, sizeof(format), "%%.%dE",
                                  fract_part.size());
        Assert(flen < sizeof(format));   // format was big enough.

        const int dlen = snprintf(data_buf, sizeof(data_buf), format, value);
        Assert(dlen < sizeof(data_buf));

        char * ep = strchr(data_buf, 'E');
        Assert(ep);
        fract_end = ep++;
        if      (*ep == '+')   ++ep;
        else if (*ep == '-')   { expo_negative = true;   ++ep; }

        int elen = strlen(ep);

        // insert leading zeros until we have at least min_len digits.
        //
        for (; elen < exponent.min_len; ++elen)   data_expo.append(UNI_ASCII_0);

        data_expo.append(UCS_string(UTF8_string(ep)));
      }
   else   // no exponent in format string.
      {
        const int flen = snprintf(format, sizeof(format), "%%.%df",
                                  fract_part.size());
        Assert(flen < sizeof(format));   // assume no snprintf() overflow

        const int dlen = snprintf(data_buf, sizeof(data_buf), format, value);
        Assert(dlen < sizeof(data_buf));
        fract_end = data_buf + dlen;
      }

char * int_end = strchr(data_buf, '.');
   if (fract_part.size() == 0)
      {
        Assert(int_end == 0);
        int_end = fract_end;
      }
   else
      {
        char * fract_digits = int_end + 1;
        Assert(int_end);

        int flen = fract_end - fract_digits;

        // remove trailing zeros, but leave at least min_len chars.
        while (fract_digits[flen - 1] == '0' && flen > fract_part.min_len)
               fract_digits[--flen] = 0;

        loop(f, flen)   data_fract.append(Unicode(fract_digits[f]));
      }

const int ilen = int_end - data_buf;

   // insert leading zeros so that we will have at least min_len digits
   // after appending the integer data.
   //
   loop(d, int_part.min_len - ilen)   data_int.append(UNI_ASCII_0);

   loop(i, ilen)   data_int.append(Unicode(data_buf[i]));

   // convert 0.xxx to .xxx
   //
   if (  data_int.size() == 1
      && data_int[0] == UNI_ASCII_0
      && int_part.min_len == 0
      && fract_part.size())   data_int.clear();

   Log(LOG_Bif_F12_FORMAT)
      {
        Q(format)
        Q(value)
        Q(data_int)
        Q(data_fract)
        Q(data_expo)
      }
}
//-----------------------------------------------------------------------------
UCS_string
Format_sub::insert_int_commas(const UCS_string & data, bool & overflow) const
{
size_t fill_pos = -1;
Unicode fill_char = UNI_ASCII_SPACE;

   if (flt_mask & (BIT_0 | BIT_8 | BIT_9))   // format has a '0', '8', or '9'
      {
        loop(f, format.size())
           {
             const Unicode fc = format[f];
              if (fc == UNI_ASCII_0 || fc == UNI_ASCII_9)
                 {
                   fill_pos = f;
                   fill_char = UNI_ASCII_0;
                   break;
                 }

              if (fc == UNI_ASCII_8)
                 {
                   fill_pos = f;
                   fill_char = Workspace::get_FC(2);
                   break;
                 }
           }
      }

   // we move backwards from the end of the format field, filling it
   // with chars from data of from format.
   //
UCS_string ucs;
size_t d = data.size();

   loop(f1, format.size())
      {
        const size_t f = format.size() - f1 - 1;
        const Unicode fc = format[f];
         if (fc == UNI_ASCII_COMMA)
            {
              ucs.append(Workspace::get_FC(1));   //  == ⎕FC[2] when ⎕IO is 1
            }
         else if (Avec::is_digit(fc))
            {
              if (d)                    ucs.append(data[--d]);
              else if (f >= fill_pos)   ucs.append(fill_char);
              else                      break;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << fc << "' at " << LOC << endl;
              break;
            }
      }

   if (d)   // format too short
      {
         if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;
         else                             overflow = true;
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   return ucs.reverse();
}
//-----------------------------------------------------------------------------
UCS_string
Format_sub::insert_fract_commas(const UCS_string & data) const
{
UCS_string ucs;
size_t d = 0;

   loop(f, format.size())
      {
        const Unicode fc = format[f];
         if (fc == UNI_ASCII_COMMA)
            {
              ucs.append(Workspace::get_FC(1));
            }
         else if (Avec::is_digit(fc))
            {
              if (d < data.size())   ucs.append(data[d++]);
              else                   break;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << fc << "' at " << LOC << endl;
            }
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   return ucs;
}
//-----------------------------------------------------------------------------
bool
Bif_F12_FORMAT::is_control_char(Unicode uni)
{
   return Avec::is_digit(uni)      ||
          (uni == UNI_ASCII_COMMA) ||
          (uni == UNI_ASCII_FULLSTOP);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::format_by_specification(Value_P A, Value_P B)
{
const Shape shape_1(1);
const Shape & shape_B = B->get_rank() ? B->get_shape() : shape_1;

const ShapeItem rows_B = shape_B.get_rows();
const ShapeItem cols_B = shape_B.get_cols();

   if (A->get_rank() > 1)   RANK_ERROR;
const ShapeItem len_A = A->is_skalar() ? 1 : A->get_shape_item(0);

   if (len_A != 1 && len_A != 2 && len_A != 2*cols_B)
      LENGTH_ERROR;

const APL_Float qct = Workspace::get_CT();
   if (shape_B.get_volume() == 0)   // empty B
      {
        ShapeItem W = 0;
        loop(c, cols_B)
           {
             if (len_A <= 2)   W += A->get_ravel(0).get_near_int(qct);
             else              W += A->get_ravel(2*c).get_near_int(qct);
           }

        Shape shape_Z = shape_B.without_axis(shape_B.get_rank() - 1);
        shape_Z.add_shape_item(W);
        const ShapeItem ec_Z = shape_Z.get_volume();

        Value_P Z(new Value(shape_Z, LOC));
        loop(z, ec_Z)   new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);

        Z->set_default(*Value::Spc_P);
        return Z;
      }

PrintBuffer pb;

   loop(col, cols_B)
       {
         uint32_t col_width;
         uint32_t precision;

         if (len_A == 1)
            {
              col_width = 0;
              precision = A->get_ravel(0).get_near_int(qct);
            }
         else if (len_A == 2)
            {
              col_width = A->get_ravel(0).get_near_int(qct);
              precision = A->get_ravel(1).get_near_int(qct);
            }
         else
            {
              col_width = A->get_ravel(2*col)    .get_near_int(qct);
              precision = A->get_ravel(2*col + 1).get_near_int(qct);
            }

         if (col_width < 0)   DOMAIN_ERROR;

         PrintBuffer pb_col(format_col_spec(col_width, precision,
                                         &B->get_ravel(col), cols_B, rows_B));

         if (col_width == 0)   pb_col.pad_l(UNI_ASCII_SPACE, 1);

         if (col)   pb.append_col(pb_col);
         else       pb = pb_col;
       }

const ShapeItem pb_w = pb.get_width(0);
const ShapeItem pb_h = pb.get_height();
Shape shape_Z(shape_B);
   shape_Z.set_last_shape_item(pb_w);

Value_P Z(new Value(shape_Z, LOC));

   loop(h, pb_h)
   loop(w, pb_w)   new (Z->next_ravel()) CharCell(pb.get_char(w, h));

   return Z;
}
//-----------------------------------------------------------------------------
PrintBuffer
Bif_F12_FORMAT::format_col_spec(int width, int precision, const Cell * cB,
                                int cols, int rows)
{
PrintBuffer ret;

bool has_char = false;
bool has_num = false;

   loop(r, rows)
      {
        const Cell & cell = cB[r*cols];
        if (cell.is_numeric())   has_num = true;
        else                     has_char = true;
      }

   loop(r, rows)
      {
        const Cell & cell = cB[r*cols];
        if (cell.is_character_cell())
           {
             UCS_string data = UCS_string(cell.get_char_value());

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
             continue;
           }

        if (cell.is_pointer_cell())
           {
             Value_P value = cell.get_pointer_value();
             UCS_string data = value->get_UCS_ravel();

             if (width && data.size() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;

                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
             continue;
           }

        if (!cell.is_real_cell())   DOMAIN_ERROR;

        const APL_Float value = cB[r*cols].get_real_value();
        if (precision >= 0)   // floating format
           {
             UCS_string data = format_spec_float(value, precision);
             if (width && data.size() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;
                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
           }
        else                  // exponential format
           {
             UCS_string data = format_spec_expo(value, - precision);
             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
          }
      }

   if (width && ret.get_width(0) < width)
      ret.pad_l(UNI_ASCII_SPACE, width - ret.get_width(0));

   return ret;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::add_row(PrintBuffer & ret, int row, bool has_char,
                        bool has_num, Unicode align_char, UCS_string & data)
{
   if (row == 0)         // first row: don't  align
      {
        ret.append_ucs(data);
      }
   else if (!has_char)   // only numbers: align at '.' or 'E'
      {
        ret.append_aligned(data, align_char);
      }
   else if (!has_num)    // only chars: align left
      {
        const int d = ret.get_width(0) - data.size();
        if      (d < 0)   ret.pad_r(UNI_ASCII_SPACE, -d);
        else if (d > 0)   data.append(UCS_string(d, UNI_ASCII_SPACE));
        ret.append_ucs(data);
      }
   else                 // chars and numbers: align right
      {
        const int d = ret.get_width(0) - data.size();
        if      (d < 0)   ret.pad_l(UNI_ASCII_SPACE, -d);
        else if (d > 0)   data = UCS_string(d, UNI_ASCII_SPACE) + data;
        ret.append_ucs(data);
      }
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::format_spec_float(APL_Float value, int precision)
{
UCS_string ret = UCS_string::from_double_fixed_prec(value, precision);

   // Note: the examples in the apl standard use a leading 0 (like  0.00)
   // while lrm shows .00 instead. We follow lrm and remove a leading 0.
   // Interestingly, the exponential format leaves a leading 0 in both
   // the standard and lrm
   //
   if (precision)   // we have a '.'
      {
        if (ret[0] == UNI_ASCII_0    &&
            ret[1] == UNI_ASCII_FULLSTOP)        // 0.xxx → .xxx
           {
             ret.remove_front();
           }
        else if (ret[0] == UNI_OVERBAR     &&
                 ret[1] == UNI_ASCII_0     &&
                 ret[2] == UNI_ASCII_FULLSTOP)   //  ¯0.x → ¯.x
           {
             ret[1] = UNI_OVERBAR;
             ret.remove_front();
           }
      }
   else if (ret.size() == 2       && 
            ret[0] == UNI_OVERBAR &&
            ret[1] == UNI_ASCII_0)               // ¯0 → 0
           {
             ret.remove_front();
           }

   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::format_spec_expo(APL_Float value, int precision)
{
  return UCS_string::from_double_expo_prec(value, precision - 1);
}
//-----------------------------------------------------------------------------
ostream &
operator<<(ostream & out, const Format_sub & fmt)
{
   return fmt.print(out);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_EXECUTE::eval_B(Value_P B)
{
   if (B->get_rank() > 1)   RANK_ERROR;

const UCS_string statement(*B.get());

   if (statement.size() == 0)   return Token(TOK_NO_VALUE);

   return execute_statement(statement);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_EXECUTE::execute_statement(const UCS_string & statement)
{
ExecuteList * fun = ExecuteList::fix(statement.no_pad(), LOC);
   if (fun == 0)   SYNTAX_ERROR;

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);

   Log(LOG_StateIndicator__push_pop)
      {
        Workspace::SI_top()->info(CERR, LOC);
      }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_UNIQUE::eval_B(Value_P B)
{
   if (B->get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_B = B->element_count();
   if (ec_B <= 1)  // 0 or 1 element: return B
      {
        Value_P Z = B->clone(LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Token B1_tok = Bif_F12_SORT_ASC::fun.eval_B(B);
   if (B1_tok.get_Class() != TC_VALUE)   return B1_tok;

const APL_Integer qio = Workspace::get_IO();
const APL_Float qct = Workspace::get_CT();
Value_P B1 = B1_tok.get_apl_val();

   // compute result size
   //
ShapeItem ec_Z = 0;   // first element is always in the result.
const Cell * last = &B->get_ravel(B1->get_ravel(0).get_int_value() - qio);
   loop(j, ec_B)
      {
        const APL_Integer idx = B1->get_ravel(j).get_int_value();
        const Cell * cj = &B->get_ravel(idx - qio);
        const bool new_element = (j == 0) || !last->equal(*cj, qct);
        if (new_element)
           {
             ++ec_Z;
             last = cj;
           }
      }

Value_P Z1(new Value(ec_Z, LOC));

   loop(j, ec_B)
      {
        const APL_Integer idx = B1->get_ravel(j).get_int_value();
        const Cell * cj = &B->get_ravel(idx - qio);
        const bool new_element = (j == 0) || !last->equal(*cj, qct);
        if (new_element)
           {
             new (Z1->next_ravel()) IntCell(idx);
             last = cj;
           }
      }

   // at this point , B[Z1] is the set of unique elements, but unfortunately
   // sorted. We sort Z1 in order to get the original order.
   //
Token Z2_tok = Bif_F12_SORT_ASC::fun.eval_B(Z1);
   Assert(Z2_tok.get_Class() == TC_VALUE);
Value_P Z2 = Z2_tok.get_apl_val();

Value_P Z3 = Z1->index(Z2);
Value_P Z4 = B->index(Z3);

   Z4->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z4);
}
//-----------------------------------------------------------------------------
