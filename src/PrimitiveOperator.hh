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

#ifndef __PRIMITIVE_OPERATOR_HH_DEFINED__
#define __PRIMITIVE_OPERATOR_HH_DEFINED__

#include "PrimitiveFunction.hh"
#include "StateIndicator.hh"

class BeamIterator;

//-----------------------------------------------------------------------------
/** The various built-in operators.
 */
class PrimitiveOperator : public PrimitiveFunction
{
public:
   /// Constructor.
   PrimitiveOperator(TokenTag tag) : PrimitiveFunction(tag) {}

   /// overloaded Function::is_operator()
   virtual bool is_operator() const   { return true; }

   /// overloaded Function::get_fun_valence()
   virtual int get_fun_valence() const   { return 2; }

   /// overloaded Function::get_oper_valence(). Most primitive operators are
   /// monadic, so we return 1 and overload dyadic operators (i.e. inner/outer
   /// product) to return 2
   virtual int get_oper_valence() const   { return 1; }

   /// evaluate the fill function with arguments A and B
   static Token fill(const Shape shape_Z, Value_P A, Function * fun,
                     Value_P B, const char * loc);
};
//-----------------------------------------------------------------------------

#endif // __PRIMITIVE_OPERATOR_HH_DEFINED__
