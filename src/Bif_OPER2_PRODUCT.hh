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

#ifndef __BIF_OPER2_PRODUCT_HH_DEFINED__
#define __BIF_OPER2_PRODUCT_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/**
   A dummy function for the product operator.
 */
class Bif_JOT : public PrimitiveFunction
{
public:
   /// Constructor.
   Bif_JOT() : PrimitiveFunction(TOK_JOT) {}

   virtual int get_oper_valence() const   { return 2; }

   static Bif_JOT           fun;             ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/** Primitive operators inner and outer product.
 */
class Bif_OPER2_PRODUCT : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER2_PRODUCT() : PrimitiveOperator(TOK_OPER2_PRODUCT) {}

   /// Overloaded Function::eval_ALRB().
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO,
                           Value_P B);

   static Bif_OPER2_PRODUCT fun;   ///< Built-in function.

protected:
   /// Compute the outer product of A and B with function RO.
   Token outer_product(Value_P A, Token & RO, Value_P B);

   /// Compute the inner product of A and B with functions LO and RO.
   Token inner_product(Value_P A, Token & LO, Token & RO, Value_P B);

   /// EOC handler for outer_product with user defined RO
   static bool eoc_outer_product(Token & token, EOC_arg & arg);

   /// EOC handler for inner_product with user defined RO
   static bool eoc_inner_product(Token & token, EOC_arg & arg);

   /// helper for RO. returns true if the final token was computed, and false
   /// if finish_outer_product shall be called again
   static Token finish_outer_product(EOC_arg & arg);

   /// helper for RO. returns true if the final token was computed, and false
   /// if finish_inner_product shall be called again
   static Token finish_inner_product(EOC_arg & arg);
};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER2_PRODUCT_HH_DEFINED__
