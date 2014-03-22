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

#ifndef __FUNOPER_HH__DEFINED__
#define __FUNOPER_HH__DEFINED__

#include "Error.hh"
#include "Function.hh"
#include "Output.hh"

//-----------------------------------------------------------------------------
/** an operator bound to one or more functions. The bound operator is then
    another operator (e.g. (+.) or a function (e.g. (+.×) or (+/).
 **/
class DerivedFunction : public Function
{
public:
   /// Constructor for DerivedFunctionCache
   DerivedFunction() : Function(TOK_FUN0)   {}

   /// Constructor (dyadic operator)
   DerivedFunction(Token & lfun, Function * dyop, Token & rfun, const char * loc);

   /// Constructor (monadic operator, no axis)
   DerivedFunction(Token & lfun, Function * monop, const char * loc);

   /// Constructor (monadic operator, with axis)
   DerivedFunction(Token & lfun, Function * monop, Value_P X, const char * loc);

   /// overloaded Function::print();
   virtual ostream & print(ostream & out) const;

   /// Overloaded Function::has_result()
   virtual int has_result() const
     { return left_fun.is_function() && left_fun.get_function()->has_result(); }

protected:
   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// overloaded Function::eval_B();
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_XB();
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AB();
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB();
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   virtual bool may_push_SI() const
      { return   oper->may_push_SI()
        || (left_fun .is_function() && left_fun .get_function()->may_push_SI())
        || (right_fun.is_function() && right_fun.get_function()->may_push_SI()); }

   /// the function (to the left of the operator)
   Token left_fun;

   /// the monadic operator (to the right of the function)
   Function * oper;

   /// the function on the right of the operator (if any)
   Token right_fun;

   /// the axis for \b mon_oper, or 0 if no axis
   Value_P axis;
};
//=============================================================================
/// a small cache for storing a few DerivedFunction objects
class DerivedFunctionCache
{
public:
   /// constructor: create empty FunOper cache
   DerivedFunctionCache();

   /// destructor
   ~DerivedFunctionCache();

   /// reset (clear) the cache
   void reset();

   /// get a cache entry
   DerivedFunction * get(const char * loc);

protected:
   /// a cache for derived functions
   DerivedFunction cache[MAX_FUN_OPER];

   /// the number of elements in \b cache
   int idx;
};
//-----------------------------------------------------------------------------
#endif // __FUNOPER_HH__DEFINED__
