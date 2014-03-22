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

#ifndef __BIF_OPER1_EACH_HH_DEFINED__
#define __BIF_OPER1_EACH_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/** Primitive operator ¨ (each).
 */
class Bif_OPER1_EACH : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER1_EACH() : PrimitiveOperator(TOK_OPER1_EACH) {}

   /// Overloaded Function::eval_LB().
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_ALB().
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   static Bif_OPER1_EACH    fun;      ///< Built-in function.

protected:
   /// function called when a sub-SI for a dyadic user defined LO returns
   static bool eoc_ALB(Token & token, EOC_arg & arg);

   /// function called when a sub-SI for a monadic user defined LO returns
   static bool eoc_LB(Token & token, EOC_arg & arg);

   /// helper for eval_LB. returns true if the final token was computed, and
   /// false if finish_eval_LB shall be called again
   static Token finish_eval_LB(EOC_arg & arg);

   /// helper for eval_ALB. returns true if the final token was computed, and
   /// false if finish_eval_ALB shall be called again
   static Token finish_eval_ALB(EOC_arg & arg);
};
//-----------------------------------------------------------------------------
#endif // __BIF_OPER1_EACH_HH_DEFINED__
