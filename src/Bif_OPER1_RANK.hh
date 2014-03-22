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

#ifndef __BIF_OPER1_RANK_HH_DEFINED__
#define __BIF_OPER1_RANK_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/** Primitive operator ⍤ (rank)
 */
class Bif_OPER1_RANK : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER1_RANK() : PrimitiveOperator(TOK_OPER1_RANK) {}

   /// Overloaded Function::eval_LB().
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_LXB().
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALB().
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_ALXB().
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   static Bif_OPER1_RANK    fun;      ///< Built-in function.

protected:
   /// function called when a sub-SI for a dyadic user defined LO returns
   static bool eoc_ALXB(Token & token, EOC_arg & arg);

   /// function called when a sub-SI for a monadic user defined LO returns
   static bool eoc_LXB(Token & token, EOC_arg & arg);

   /// compute \b rk_A and \b rk_B from \b X
   void compute_ranks(Value_P X, Rank & rk_A, Rank & rk_B);

   /// helper for eval_LXB. returns true if the final token was computed, and
   /// false if finish_eval_LXB shall be called again
   static Token finish_eval_LXB(EOC_arg & arg);

   /// helper for eval_ALXB. returns true if the final token was computed, and
   /// false if finish_eval_ALXB shall be called again
   static Token finish_eval_ALXB(EOC_arg & arg);
};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER1_RANK_HH_DEFINED__
