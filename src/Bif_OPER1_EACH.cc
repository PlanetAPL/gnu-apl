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

#include "Bif_OPER1_EACH.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER1_EACH     Bif_OPER1_EACH::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
   // dyadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);


   if (A->is_empty() || B->is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P Fill_A = Bif_F12_TAKE::fun.eval_B(A).get_apl_val();
        Value_P Fill_B = Bif_F12_TAKE::fun.eval_B(B).get_apl_val();
        Shape shape_Z;

        if (A->is_empty())          shape_Z = A->get_shape();
        else if (!A->is_skalar())   DOMAIN_ERROR;

        if (B->is_empty())          shape_Z = B->get_shape();
        else if (!B->is_skalar())   DOMAIN_ERROR;

        Value_P Z1 = LO->eval_fill_AB(Fill_A, Fill_B).get_apl_val();

        Value_P Z(new Value(shape_Z, LOC));
        new (&Z->get_ravel(0)) PointerCell(Z1);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

EOC_arg arg(B);   arg.A = A;
EACH_ALB & _arg = arg.u.u_EACH_ALB;

   _arg.dA = 1;
   _arg.LO = LO;
   _arg.dB = 1;


   if (A->nz_element_count() == 1)
      {
        if (B->is_skalar_or_len1_vector())   return LO->eval_AB(A, B);

        _arg.count = B->element_count();

        _arg.dA = 0;
        if (LO->has_result())
           arg.Z = Value_P(new Value(B->get_shape(), LOC));
      }
   else if (B->nz_element_count() == 1)
      {
        _arg.dB = 0;
        _arg.count = A->element_count();
        if (LO->has_result())
           arg.Z = Value_P(new Value(A->get_shape(), LOC));
      }
   else if (A->same_shape(B))
      {
        _arg.count = B->element_count();
        if (LO->has_result())
           arg.Z = Value_P(new Value(A->get_shape(), LOC));
      }
   else
      {
        if (A->same_rank(B))   LENGTH_ERROR;
        RANK_ERROR;
      }

   _arg.cA = &A->get_ravel(0);
   _arg.cB = &B->get_ravel(0);

   // arg.Z can be 0 (if not x(LO->has_result())

   _arg.how = 0;
   return finish_eval_ALB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_ALB(EOC_arg & arg)
{
EACH_ALB & _arg = arg.u.u_EACH_ALB;

   if (_arg.how == 1)   goto how_1;

how_0:
   Assert1(_arg.how == 0);
   _arg.z = 0;

loop_z:
   {
     Value_P LO_A = _arg.cA->to_value(LOC);     // left argument of LO
     Value_P LO_B = _arg.cB->to_value(LOC);     // right argument of LO;
     _arg.sub = !(_arg.cA->is_pointer_cell() || _arg.cB->is_pointer_cell());

     _arg.cA += _arg.dA;
     _arg.cB += _arg.dB;

     Token result = _arg.LO->eval_AB(LO_A, LO_B);

     // if LO was a primitive function, then result may be a value.
     // if LO was a user defined function then result may be TOK_SI_PUSHED.
     // in both cases result could be TOK_ERROR.
     //
     if (result.get_Class() == TC_VALUE)
        {
          Value_P vZ = result.get_apl_val();
          if (_arg.sub)   new (arg.Z->next_ravel())  PointerCell(vZ);
          else           arg.Z->next_ravel()->init_from_value(vZ, LOC);

          goto next_z;   // next z
        }

     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_ALB);
          Workspace::SI_top()->get_eoc_arg() = arg;

          return result;   // continue in user defined function...
        }

     Q(result);   FIXME;
   }

how_1:
next_z:
   if (++_arg.z < _arg.count)   goto loop_z;

   if (!!arg.Z)   // LO with result
      {
        arg.Z->set_default(*arg.B.get());
        arg.Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, arg.Z);
      }


   return Token(TOK_VOID);   // LO without result
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_ALB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
EACH_ALB & _arg = arg.u.u_EACH_ALB;

   if (!!arg.Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

       Value_P vZ = token.get_apl_val();
       if (_arg.sub)   new (arg.Z->next_ravel())  PointerCell(vZ);
       else           arg.Z->next_ravel()->init_from_value(vZ, LOC);
      }
   else        // LO without result, maybe successful
      {
        if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (_arg.z < (_arg.count - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_eval_ALB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_LB(Token & _LO, Value_P B)
{
   // monadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);

EOC_arg arg(B);
EACH_LB & _arg = arg.u.u_EACH_LB;

   _arg.LO = LO;
   _arg.cB = &B->get_ravel(0);
   if (LO->has_result())   arg.Z = Value_P(new Value(B->get_shape(), LOC));

   _arg.count = B->element_count();

   _arg.how = 0;
   return finish_eval_LB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_LB(EOC_arg & arg)
{
EACH_LB & _arg = arg.u.u_EACH_LB;

   if (_arg.how == 1)   goto how_1;

how_0:
   Assert1(_arg.how == 0);
   _arg.z = 0;

loop_z:
   {
     Value_P LO_B;         // right argument of LO;
     bool cup_B = false;   // clean-up B ?

     if (_arg.cB->is_pointer_cell())
        {
          LO_B = _arg.cB++->get_pointer_value();
          _arg.sub = true;
        }
     else
        {
          LO_B = Value_P(new Value(LOC));

          LO_B->get_ravel(0).init(*_arg.cB++);
          LO_B->set_complete();
          cup_B = true;   // clean-up B
          _arg.sub = false;
        }

     Token result = _arg.LO->eval_B(LO_B);

     if (cup_B)
        {
        }

     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_LB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     // if LO was a primitive function, then result may be a value.
     // if LO was a user defined function then result may be TOK_SI_PUSHED.
     // in both cases result could be TOK_ERROR.
     //
     if (result.get_Class() == TC_VALUE)
        {
          Value_P vZ = result.get_apl_val();

          if (_arg.sub)   new (arg.Z->next_ravel())   PointerCell(vZ);
          else           arg.Z->next_ravel()->init_from_value(vZ, LOC);

          goto next_z;
        }

     if (result.get_tag() == TOK_VOID)   goto next_z;

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_z:
   if (++_arg.z < _arg.count)   goto loop_z;

   if (!!arg.Z)   // LO with result
      {
        arg.Z->set_default(*arg.B.get());
        arg.Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, arg.Z);
      }

   return Token(TOK_VOID);   // LO without result
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_LB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
EACH_LB & _arg = arg.u.u_EACH_LB;

   if (!!arg.Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

        Value_P vZ = token.get_apl_val();
        if (_arg.sub)   new (arg.Z->next_ravel())  PointerCell(vZ);
        else           arg.Z->next_ravel()->init_from_value(vZ, LOC);
      }
   else        // LO without result, maybe successful
      {
       if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (_arg.z < (_arg.count - 1))   Workspace::pop_SI(LOC);
   copy_1(token, finish_eval_LB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------

