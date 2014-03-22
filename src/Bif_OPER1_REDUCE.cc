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
#include "Workspace.hh"

Bif_OPER1_REDUCE   Bif_OPER1_REDUCE::fun;
Bif_OPER1_REDUCE1  Bif_OPER1_REDUCE1::fun;

//-----------------------------------------------------------------------------
Token
Bif_REDUCE::replicate(Value_P A, Value_P B, Axis axis)
{
   // turn skalar B into ,B
   //
Shape shape_B = B->get_shape();
   if (shape_B.get_rank() == 0)
      {
         shape_B.add_shape_item(1);
         axis = 0;
      }

   if (A->get_rank() > 1)             RANK_ERROR;
   if (axis >= shape_B.get_rank())    INDEX_ERROR;

const APL_Float qct = Workspace::get_CT();
const ShapeItem len_B = shape_B.get_shape_item(axis);
ShapeItem len_A = A->element_count();
ShapeItem len_Z = 0;
vector<ShapeItem> rep_counts;
   rep_counts.reserve(len_B);
   if (len_A == 1)   // single a -> a a ... a (len_B times)
      {
        len_A = len_B;
        APL_Integer rep_A = A->get_ravel(0).get_near_int(qct);
        loop(a, len_A)   rep_counts.push_back(rep_A);
        if (rep_A > 0)        len_Z =  rep_A*len_B;
        else if (rep_A < 0)   len_Z = -rep_A*len_B;
      }
   else
      {
        ShapeItem geq_A = 0;   // number of items >= 0 in A
        loop(a, len_A)
           {
             APL_Integer rep_A = A->get_ravel(a).get_near_int(qct);
             rep_counts.push_back(rep_A);
             if (rep_A > 0)        { len_Z += rep_A;   ++geq_A; }
             else if (rep_A < 0)   len_Z -= rep_A;
             else                  ++geq_A;
           }

        if (len_B != 1 && geq_A != len_B)   LENGTH_ERROR;
      }

Shape shape_Z(shape_B);
   shape_Z.set_shape_item(axis, len_Z);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_B3(shape_B, axis);

   loop(h, shape_B3.h())
      {
        ShapeItem bm = 0;
        loop(m, rep_counts.size())
           {
             const ShapeItem rep = rep_counts[m];

             if (rep >= 0)   // copy l*rep items
                {
                  loop(r, rep)
                  loop(l, shape_B3.l())
                     {
                       Z->next_ravel()->init(B->get_ravel(shape_B3.hml(h, bm, l)));
                     }
                  if (shape_B3.m() > 1)   ++bm;
                }
             else  // init l*-rep items with the fill item
                {
                  loop(r, -rep)
                  loop(l, shape_B3.l())
                     {
                       Z->next_ravel()->init_type(B->get_ravel(shape_B3.hml(h, 0, l)));
                     }

                  // cB is not incremented when fill item is used.
                }
           }
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::reduce(Function * LO, Value_P B, Axis axis)
{
   Assert1(LO);
   if (!LO->has_result())   DOMAIN_ERROR;

   // if B is a skalar, then Z is B.
   //
   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (axis >= B->get_rank())   AXIS_ERROR;

const ShapeItem m_len = B->get_shape_item(axis);
const APL_Float qct = Workspace::get_CT();

const Shape shape_Z = B->get_shape().without_axis(axis);

   if (m_len == 0)   // apply the identity function
      {
        return LO->eval_identity_fun(B, axis);
      }

   if (m_len == 1)   return Bif_F12_RHO::do_reshape(shape_Z, B);

const Shape3 B3(B->get_shape(), axis);
const Shape3 Z3(B3.h(), 1, B3.l());
   return do_reduce(shape_Z, Z3, B3.m(), LO, axis, B, B->get_shape_item(axis));
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::reduce_n_wise(Value_P A, Function * LO, Value_P B, Axis axis)
{
   if (!LO->has_result())   DOMAIN_ERROR;

   if (A->element_count() != 1)   LENGTH_ERROR;
const APL_Integer A0 = A->get_ravel(0).get_int_value();
const int n_wise = A0 < 0 ? -A0 : A0;   // the number of items

   if (B->is_skalar())
      {
        if (n_wise > 1)                               DOMAIN_ERROR;
      }
   else
      {
        if (n_wise > (1 + B->get_shape_item(axis)))   DOMAIN_ERROR;
      }

   Assert1(LO);

   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (axis >= B->get_rank())   AXIS_ERROR;

   if (n_wise == 0)   // apply the identity function
      {
        Shape shape_B1 = B->get_shape().insert_axis(axis, 0);
        shape_B1.increment_shape_item(axis + 1);
        Value_P val(new Value(shape_B1, LOC));
        val->get_ravel(0).init(B->get_ravel(0));   // prototype

        Token result = LO->eval_identity_fun(val, axis);
        return result;
      }

   if (n_wise == (1 + B->get_shape_item(axis)))   // empty result
      {
        // the examples in ISO/IEC 13751 (2000) p. 116, as well as lrm,
        // suggest an empty result, while the algorithm on pp. 115/116 seems
        // not to work for n_wise == (1 + B->get_shape_item(axis)).
        // We follow the examples and lrm.
        //
        Shape shape_Z = B->get_shape();
        shape_Z.set_shape_item(axis, 0);
        Value_P Z(new Value(shape_Z, LOC));
        Z->get_ravel(0).init(B->get_ravel(0));
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) - n_wise + 1);

   if (n_wise == 1)   return Bif_F12_RHO::fun.do_reshape(shape_Z, B);

const Shape3 Z3(shape_Z, axis);
   return do_reduce(shape_Z, Z3, A0, LO, axis, B, B->get_shape_item(axis));
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::do_reduce(const Shape & shape_Z, const Shape3 & Z3, ShapeItem a,
                      Function * LO, Axis axis, Value_P B, ShapeItem bm)
{
   if (shape_Z.is_empty())   return LO->eval_identity_fun(B, axis);

Value_P Z(new Value(shape_Z, LOC));
EOC_arg arg(Z, B);
REDUCTION & _arg = arg.u.u_REDUCTION;

   _arg.init(Z3, LO, &B->get_ravel(0), bm, a, 0);

Token tok(TOK_FIRST_TIME);
   eoc_beam(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
bool
Bif_REDUCE::eoc_beam(Token & token, EOC_arg & si_arg)
{
   if (token.get_tag() == TOK_ERROR)   return false;   // stop it

EOC_arg arg = si_arg;
REDUCTION & _arg = arg.u.u_REDUCTION;

   if (token.get_tag() == TOK_FIRST_TIME)   // first call to eoc_beam()
      {
new_beam:
        const Cell * cB = _arg.beam.next_B();

        // we need a token with a free (erasable) value. If the value were
        // nested, then we would get a double delete (from the original owner
        // and from our result). We can't therefore call cB->to_value() for
        // nested values.
        //
        if (cB->is_pointer_cell())
           copy_1(token, Token(TOK_APL_VALUE1,
                                cB->get_pointer_value()->clone(LOC)), LOC);
        else
           copy_1(token, Token(TOK_APL_VALUE1, cB->to_value(LOC)), LOC);
      }

again:
Value_P BB = token.get_apl_val();

   if (_arg.beam.done())   // last reduction in current beam
      {
        _arg.frame.next_hml();

        // if beam has only one element, as for the first column in scan,
        // then BB->clear_eoc() below is never reached and we have to do
        // it here.
        //
        arg.Z->next_ravel()->init_from_value(BB, LOC);

        if (_arg.frame.done())   // if last beam (final result complete)
           {
             arg.Z->check_value(LOC);
             copy_1(token, Token(TOK_APL_VALUE1, arg.Z), LOC);
             return false;   // stop it
           }

        if (_arg.frame.A0_inc)   _arg.beam.length = _arg.frame.m + 1;
        const Cell * beam = _arg.frame.beam_start();
        _arg.beam.reset(beam);
        goto new_beam;
      }

   // pop context for previous eval_AB() call
   //
   if (_arg.need_pop)   Workspace::pop_SI(LOC);

const Cell * cA = _arg.beam.next_B();
Value_P AA = cA->to_value(LOC);
   copy_1(token, _arg.beam.LO->eval_AB(AA, BB), LOC);

   // if token is an APL value, then LO was a primitive function and the last
   // LO->eval_AB() succeeded. No SI entry was pushed, so we can loop locally.
   //
   if (token.get_Class() == TC_VALUE)   goto again;

   // if eval_AB() returned an error then stop. This can happen for both
   // primitive and user-defined LO.
   //
   if (token.get_tag() == TOK_ERROR)   return false;   // stop it

   // Otherwise LO must have been a user defined function
   //
   Assert(token.get_tag() == TOK_SI_PUSHED);
   _arg.need_pop = true;

   Workspace::SI_top()->set_eoc_handler(eoc_beam);
   Workspace::SI_top()->get_eoc_arg() = arg;
   return true;   // continue
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());

   return replicate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_LXB(Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

const Rank axis = X->get_single_axis(B->get_rank());
   return reduce(LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_ALXB(Value_P A, Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

const Rank axis = X->get_single_axis(B->get_rank());
   return reduce_n_wise(A, LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_AXB(Value_P A,
                            Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());

   return replicate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_LXB(Token & LO, Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());
   return reduce(LO.get_function(), B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
const Rank axis = X->get_single_axis(B->get_rank());
   return reduce_n_wise(A, LO.get_function(), B, axis);
}
//-----------------------------------------------------------------------------
