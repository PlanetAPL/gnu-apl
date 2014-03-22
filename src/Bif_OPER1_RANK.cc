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

#include "Bif_OPER1_RANK.hh"
#include "IntCell.hh"
#include "Workspace.hh"

Bif_OPER1_RANK     Bif_OPER1_RANK::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_LB(Token & _LO, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

   // the rank operator always needs an axis. We convert the
   // weird Sharp syntax of A ⍤ j B into A ⍤[j] B
   //
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector  j1 ⊂B  or  j1 j2 ⊂B  or  j1 j2 j3 ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int axes_count = B->element_count() - 1;

   if (axes_count < 1)   AXIS_ERROR;
   if (axes_count > 3)   AXIS_ERROR;

Value_P arg  = B->get_ravel(axes_count).get_pointer_value();
Value_P X(new Value(axes_count, LOC));
   loop(a, axes_count)   
   new (&X->get_ravel(a))   IntCell(B->get_ravel(a).get_int_value());

Token ret = eval_LXB(_LO, X, arg);
   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_LXB(Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_low = -1;   // indicate monadic
Rank rk_B_low = B->get_rank();
   compute_ranks(X, rk_A_low, rk_B_low);

   // split shape of B into high and low shapes.
   //
EOC_arg arg(B);
RANK_LXB & _arg = arg.u.u_RANK_LXB;

   _arg.get_sh_B_low() = B->get_shape().low_shape(rk_B_low);
   _arg.get_sh_B_high() = B->get_shape().high_shape(B->get_rank() - rk_B_low);
   new (&_arg.get_sh_Z_max_low()) Shape;
   _arg.ec_high = _arg.get_sh_B_high().get_volume();
   _arg.ec_B_low = _arg.get_sh_B_low().get_volume();
   _arg.cB = &B->get_ravel(0);
   _arg.LO = LO;
   _arg.ZZ = new Value_P[_arg.ec_high];
   _arg.how = 0;

   return finish_eval_LXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::finish_eval_LXB(EOC_arg & arg)
{
RANK_LXB & _arg = arg.u.u_RANK_LXB;

   if (_arg.how == 1)   goto how_1;

how_0:
   Assert1(_arg.how == 0);
   _arg.h = 0;

loop_h:
   {
     Value_P BB(new Value(_arg.get_sh_B_low(), LOC));
     Assert1(_arg.ec_B_low == _arg.get_sh_B_low().get_volume());
     loop(l, _arg.ec_B_low)   BB->get_ravel(l).init(*_arg.cB++);
     BB->check_value(LOC);

     Token result = _arg.LO->eval_B(BB);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_LXB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_Class() == TC_VALUE)
        {
          _arg.ZZ[_arg.h] = result.get_apl_val();

          // adjust result rank to ZZ if needed
          //
          _arg.get_sh_Z_max_low().expand(_arg.ZZ[_arg.h]->get_shape());

          goto next_h;
       }

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_h:
   if (++_arg.h < _arg.ec_high)   goto loop_h;

const ShapeItem ec_Z_max_low = _arg.get_sh_Z_max_low().get_volume();
Shape sh_Z = _arg.get_sh_B_high() + _arg.get_sh_Z_max_low();
Value_P Z(new Value(sh_Z, LOC));

   loop(hh, _arg.ec_high)
      {
        // sh_Z_max_low ↑ ZZ  if necessary
        //
        if (_arg.get_sh_Z_max_low() != _arg.ZZ[hh]->get_shape())
           {
             Assert(ec_Z_max_low >= _arg.ZZ[hh]->element_count());
             Token zz = Bif_F12_TAKE::fun.do_take(_arg.get_sh_Z_max_low(),
                                                  _arg.ZZ[hh]);
             _arg.ZZ[hh] = zz.get_apl_val();
           }

        // copy partial result into final result.
        //
        Cell * cZZ = &_arg.ZZ[hh]->get_ravel(0);
        loop(z, ec_Z_max_low)
           {
             Z->next_ravel()->init(*cZZ);
             new (cZZ++) IntCell(0);
           }
      }

   delete [] _arg.ZZ;
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_RANK::eoc_LXB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
RANK_LXB & _arg = arg.u.u_RANK_LXB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     _arg.ZZ[_arg.h] = token.get_apl_val();
     // adjust result rank to ZZ if needed
     //
     _arg.get_sh_Z_max_low().expand(_arg.ZZ[_arg.h]->get_shape());
   }

   if (_arg.h < (_arg.ec_high - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_eval_LXB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

   // the rank operator always needs an axis. We convert the
   // weird Sharp syntax of A ⍤ j B into A ⍤[j] B
   //
   // B is expected to be "glued" by the prefix parser, which should
   // result in a 2, 3, or 4 element vector j1 ⊂B  or  j1 j2 ⊂B  or  j1 j2 j3 ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int axes_count = B->element_count() - 1;

   if (axes_count < 1)   AXIS_ERROR;
   if (axes_count > 3)   AXIS_ERROR;

Value_P arg  = B->get_ravel(axes_count).get_pointer_value();
Value_P X(new Value(axes_count, LOC));
   loop(a, axes_count)   
   new (&X->get_ravel(a))   IntCell(B->get_ravel(a).get_int_value());

Token ret = eval_ALXB(A, _LO, X, arg);
   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_ALXB(Value_P A, Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_low = A->get_rank();   // not used
Rank rk_B_low = B->get_rank();
   compute_ranks(X, rk_A_low, rk_B_low);

Rank rk_A_high = A->get_rank() - rk_A_low;   // rk_A_high is y8
Rank rk_B_high = B->get_rank() - rk_B_low;   // rk_B_high is y9

   // if both high-ranks are 0, then return A LO B.
   //
   if (rk_A_high == 0 && rk_B_high == 0)   return LO->eval_AB(A, B);

   // split shapes of A1 and B1 into high and low shapes. Even though A1 and B1
   // have the same shape, rk_A_high and rk_B_high could be different, leading
   // to different split shapes for A1 and B1
   //
EOC_arg arg(B);   arg.A = A;
RANK_ALXB & _arg = arg.u.u_RANK_ALXB;

   _arg.get_sh_A_low() = A->get_shape().low_shape(rk_A_low);
Shape sh_A_high = A->get_shape().high_shape(A->get_rank() - rk_A_low);
   _arg.get_sh_B_low() = B->get_shape().low_shape(rk_B_low);
   _arg.get_sh_B_high() = B->get_shape().high_shape(B->get_rank() - rk_B_low);
   new (&_arg.get_sh_Z_max_low())   Shape;

   // a trick to avoid conforming A to B or B to A. If A or B
   // needs to be conformed, then we set the corresponding repeat_A or 
   // repeat_B to true and copy the same A or B again and again
   //
   _arg.repeat_A = (rk_A_high == 0);
   _arg.repeat_B = (rk_B_high == 0);

   if (_arg.repeat_A)   // "conform" A to B
      {
         rk_A_high = rk_B_high;
         sh_A_high = _arg.get_sh_B_high();
      }
   else if (_arg.repeat_B)   // "conform" B to A
      {
         rk_B_high = rk_A_high;
         _arg.get_sh_B_high() = sh_A_high;
      }
   else                       // check lengths
      {
        if (rk_A_high != rk_B_high)              RANK_ERROR;
        if (sh_A_high != _arg.get_sh_B_high())   LENGTH_ERROR;
      }
 
   // at this point sh_A_high == sh_B_high, so we can use either of them
   // to compute ec_A_low
   //
   _arg.ec_high = _arg.get_sh_B_high().get_volume();
   _arg.ec_A_low = _arg.get_sh_A_low().get_volume();
   _arg.ec_B_low = _arg.get_sh_B_low().get_volume();

   _arg.ZZ = new Value_P[_arg.ec_high];
   arg.A = A;
   _arg.cA = &A->get_ravel(0);
   _arg.LO = LO;
   arg.B = B;
   _arg.cB = &B->get_ravel(0);

   _arg.how = 0;
   return finish_eval_ALXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::finish_eval_ALXB(EOC_arg & arg)
{
RANK_ALXB & _arg = arg.u.u_RANK_ALXB;

   if (_arg.how == 1)   goto how_1;

how_0:
   Assert1(_arg.how == 0);
   _arg.h = 0;

loop_h:
   {
     Value_P AA(new Value(_arg.get_sh_A_low(), LOC));
     Value_P BB(new Value(_arg.get_sh_B_low(), LOC));

     loop(l, _arg.ec_A_low)   AA->get_ravel(l).init(*_arg.cA++);
     loop(l, _arg.ec_B_low)   BB->get_ravel(l).init(*_arg.cB++);

     if (_arg.repeat_A)   _arg.cA = &arg.A->get_ravel(0);
     if (_arg.repeat_B)   _arg.cB = &arg.B->get_ravel(0);

     AA->check_value(LOC);
     BB->check_value(LOC);

     Token result = _arg.LO->eval_AB(AA, BB);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_ALXB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_Class() == TC_VALUE)
        {
          _arg.ZZ[_arg.h] = result.get_apl_val();

          // adjust result rank to ZZ if needed
          //
          _arg.get_sh_Z_max_low().expand(_arg.ZZ[_arg.h]->get_shape());

          goto next_h;
       }

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_h:
   if (++_arg.h < _arg.ec_high)   goto loop_h;

const ShapeItem ec_Z_max_low = _arg.get_sh_Z_max_low().get_volume();
Shape sh_Z = _arg.get_sh_B_high() + _arg.get_sh_Z_max_low();
Value_P Z(new Value(sh_Z, LOC));

   loop(hh, _arg.ec_high)
      {
        // sh_Z_max_low ↑ ZZ  if necessary
        //
        if (_arg.get_sh_Z_max_low() != _arg.ZZ[hh]->get_shape())
           {
             Assert(ec_Z_max_low >= _arg.ZZ[hh]->element_count());
             Token zz = Bif_F12_TAKE::fun.do_take(_arg.get_sh_Z_max_low(),
                                                  _arg.ZZ[hh]);
             _arg.ZZ[hh] = zz.get_apl_val();
           }

        // copy partial result into final result.
        //
        Cell * cZZ = &_arg.ZZ[hh]->get_ravel(0);
        loop(z, ec_Z_max_low)
           {
             Z->next_ravel()->init(*cZZ);
             new (cZZ++) IntCell(0);
           }
      }

   delete [] _arg.ZZ;
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_RANK::eoc_ALXB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
RANK_ALXB & _arg = arg.u.u_RANK_ALXB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     _arg.ZZ[_arg.h] = token.get_apl_val();

     // adjust result rank to ZZ if needed
     //
     _arg.get_sh_Z_max_low().expand(_arg.ZZ[_arg.h]->get_shape());
   }

   if (_arg.h < (_arg.ec_high - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_eval_ALXB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
void
Bif_OPER1_RANK::compute_ranks(Value_P X,
                              /* in/out */  Rank & rk_A_low,
                              /* in/out */  Rank & rk_B_low)
{
const Rank rk_A = rk_A_low;
const Rank rk_B = rk_B_low;

   if (!X)            AXIS_ERROR;
   if (X->get_rank() > 1)   DOMAIN_ERROR;

const APL_Float qct = Workspace::get_CT();

   if (rk_A == -1)   // monadic
      {
        switch(X->element_count())
           {
             case 1:  rk_B_low = X->get_ravel(0).get_near_int(qct);   break;

             case 2:             X->get_ravel(0).get_near_int(qct);
                      rk_B_low = X->get_ravel(1).get_near_int(qct);   break;

             case 3:  rk_B_low = X->get_ravel(0).get_near_int(qct);
                                 X->get_ravel(1).get_near_int(qct);
                                 X->get_ravel(2).get_near_int(qct);   break;

             default: LENGTH_ERROR;
           }
      }
   else              // dyadic
      {
        switch(X->element_count())
           {
             case 1:  rk_A_low = X->get_ravel(0).get_near_int(qct);
                      rk_B_low = rk_A_low;                               break;

             case 2:  rk_A_low = X->get_ravel(0).get_near_int(qct);
                      rk_B_low = X->get_ravel(1).get_near_int(qct);   break;

             case 3:             X->get_ravel(0).get_near_int(qct);
                      rk_A_low = X->get_ravel(1).get_near_int(qct);
                      rk_B_low = X->get_ravel(2).get_near_int(qct);   break;

             default: LENGTH_ERROR;
           }
      }

   if (rk_A_low > rk_A)   rk_A_low = rk_A;
   if (rk_A_low < 0)      rk_A_low += rk_A;
   if (rk_A_low < 0)      rk_A_low = 0;

   if (rk_B_low > rk_B)   rk_B_low = rk_B;
   if (rk_B_low < 0)      rk_B_low += rk_B;
   if (rk_B_low < 0)      rk_B_low = 0;
}
//-----------------------------------------------------------------------------
