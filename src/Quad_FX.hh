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

#ifndef __QUAD_FX_HH_DEFINED__
#define __QUAD_FX_HH_DEFINED__

#include "QuadFunction.hh"

//-----------------------------------------------------------------------------
/**
   The system function Quad-FX (Fix)
 */
class Quad_FX : public QuadFunction
{
public:
   /// Constructor.
   Quad_FX() : QuadFunction(TOK_QUAD_FX) {}

   static Quad_FX           fun;          ///< Built-in function.

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// do ⎕FX with execution properties \b exec_props
   Token do_quad_FX(const int * exec_props, const UCS_string & text);

protected:
   /// do ⎕FX with execution properties \b exec_props
   Token do_quad_FX(const int * exec_props, Value_P B);

   /// ⎕FX with native function and optional library reference
   Token do_native_FX(Value_P A, Axis axis, Value_P B);
};
//-----------------------------------------------------------------------------
#endif //  __QUAD_FX_HH_DEFINED__

