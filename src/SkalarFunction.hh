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

#ifndef __SKALAR_FUNCTION_HH_DEFINED__
#define __SKALAR_FUNCTION_HH_DEFINED__

#include "PrimitiveFunction.hh"
#include "Value.hh"
#include "Id.hh"

typedef void (Cell::*prim_f1)(Cell *) const;
typedef void (Cell::*prim_f2)(Cell *, const Cell *) const;

//-----------------------------------------------------------------------------
/** Base class for skalar functions (functions whose monadic and/or dyadic
    version are skalar.
 */
class SkalarFunction : public PrimitiveFunction
{
public:
   /// Construct a SkalarFunction with \b Id \b id in
   SkalarFunction(TokenTag tag) : PrimitiveFunction(tag) {}

   /// Evaluate a skalar function monadically.
   Token eval_skalar_B(Value_P B, prim_f1 fun);

   /// Evaluate a skalar function dyadically.
   Token eval_skalar_AB(Value_P A, Value_P B, prim_f2 fun);

   /// Evaluate a skalar function dyadically with axis.
   Token eval_skalar_AXB(Value_P A, Value_P X, Value_P B, prim_f2 fun);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::has_result()
   virtual int has_result() const   { return 1; }

protected:
   /// Apply a function to a nested sub array.
   void expand_pointers(Cell * cell_Z, const Cell * cell_A,
                        const Cell * cell_B, prim_f2 fun);

   /// A helper function for eval_skalar_AXB().
   Token eval_skalar_AXB(Value_P A, bool * axis_present,
                          Value_P B, prim_f2 fun, bool reversed);

   /// Evaluate \b the identity function.
   Token eval_skalar_identity_fun(Value_P B, Axis axis, Value_P FI0);
};
//-----------------------------------------------------------------------------
/** Skalar functions binomial and factorial.
 */
class Bif_F12_BINOM : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_BINOM() : SkalarFunction(TOK_F12_BINOM) {}

   static Bif_F12_BINOM     fun;       ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_factorial); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_binomial); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_binomial); }
};
//-----------------------------------------------------------------------------
/** Skalar function less than.
 */
class Bif_F2_LESS : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_LESS() : SkalarFunction(TOK_F2_LESS) {}

   static Bif_F2_LESS       fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_less_than); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_less_than); }
};
//-----------------------------------------------------------------------------
/** Skalar function equal.
 */
class Bif_F2_EQUAL : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_EQUAL() : SkalarFunction(TOK_F2_EQUAL) {}

   static Bif_F2_EQUAL      fun;        ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_equal); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_equal); }
};
//-----------------------------------------------------------------------------
/** Skalar function greater than.
 */
class Bif_F2_GREATER : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_GREATER() : SkalarFunction(TOK_F2_GREATER) {}

   static Bif_F2_GREATER    fun;      ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_greater_than); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_greater_than); }
};
//-----------------------------------------------------------------------------
/** Skalar function and.
 */
class Bif_F2_AND : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_AND() : SkalarFunction(TOK_F2_AND) {}

   static Bif_F2_AND        fun;          ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_and); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_and); }
};
//-----------------------------------------------------------------------------
/** Skalar function or.
 */
class Bif_F2_OR : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_OR() : SkalarFunction(TOK_F2_OR) {}

   static Bif_F2_OR         fun;           ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_or); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_or); }
};
//-----------------------------------------------------------------------------
/** Skalar function less or equal.
 */
class Bif_F2_LEQ : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_LEQ() : SkalarFunction(TOK_F2_LEQ) {}

   static Bif_F2_LEQ        fun;          ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_less_eq); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_less_eq); }
};
//-----------------------------------------------------------------------------
/** Skalar function equal.
 */
class Bif_F2_MEQ : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_MEQ() : SkalarFunction(TOK_F2_MEQ) {}

   static Bif_F2_MEQ        fun;          ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_greater_eq); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_greater_eq); }
};
//-----------------------------------------------------------------------------
/** Skalar function not equal
 */
class Bif_F2_UNEQ : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_UNEQ() : SkalarFunction(TOK_F2_UNEQ) {}

   static Bif_F2_UNEQ       fun;         ///< Built-in function.

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_not_equal); }

protected:
   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_not_equal); }
};
//-----------------------------------------------------------------------------
/** Skalar function find.
 */
class Bif_F2_FIND : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_FIND() : SkalarFunction(TOK_F2_FIND) {}

   static Bif_F2_FIND       fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Return true iff A is contained in B.
   static bool contained(const Shape & shape_A, const Cell * cA,
                         Value_P B, const Shape & idx_B, double qct);
};
//-----------------------------------------------------------------------------
/** Skalar function nor.
 */
class Bif_F2_NOR : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_NOR() : SkalarFunction(TOK_F2_NOR) {}

   static Bif_F2_NOR        fun;          ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_nor); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_nor); }
};
//-----------------------------------------------------------------------------
/** Skalar function nand.
 */
class Bif_F2_NAND : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F2_NAND() : SkalarFunction(TOK_F2_NAND) {}

   static Bif_F2_NAND       fun;         ///< Built-in function.

protected:
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_nand); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_nand); }
};
//-----------------------------------------------------------------------------
/** Skalar functions power and exponential.
 */
class Bif_F12_POWER : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_POWER() : SkalarFunction(TOK_F12_POWER) {}

   static Bif_F12_POWER     fun;       ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_exponential); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_power); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_power); }
};
//-----------------------------------------------------------------------------
/** Skalar functions add and conjugate.
 */
class Bif_F12_PLUS : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_PLUS() : SkalarFunction(TOK_F12_PLUS) {}

   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_conjugate); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_add); }

   static Bif_F12_PLUS      fun;        ///< Built-in function.

protected:
   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_add); }
};
//-----------------------------------------------------------------------------
/** Skalar functions subtrct and negative.
 */
class Bif_F12_MINUS : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_MINUS() : SkalarFunction(TOK_F12_MINUS) {}

   static Bif_F12_MINUS     fun;       ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_negative); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_subtract); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_subtract); }
};
//-----------------------------------------------------------------------------
/** Skalar function roll and non-skalar function dial.
 */
class Bif_F12_ROLL : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_ROLL() : SkalarFunction(TOK_F12_ROLL) {}

   static Bif_F12_ROLL      fun;        ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// dial A from B.
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** Skalar function not and non-skalar function without.
 */
class Bif_F12_WITHOUT : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_WITHOUT() : SkalarFunction(TOK_F12_WITHOUT) {}

   static Bif_F12_WITHOUT   fun;     ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_not); }

   /// Compute A without B.
   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** Skalar functions times and direction.
 */
class Bif_F12_TIMES : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_TIMES() : SkalarFunction(TOK_F12_TIMES) {}

   static Bif_F12_TIMES     fun;       ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_direction); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_multiply); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_multiply); }
};
//-----------------------------------------------------------------------------
/** Skalar functions divide and reciprocal.
 */
class Bif_F12_DIVIDE : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_DIVIDE() : SkalarFunction(TOK_F12_DIVIDE) {}

   static Bif_F12_DIVIDE    fun;      ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_reciprocal); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_divide); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::One_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_divide); }
};
//-----------------------------------------------------------------------------
/** Skalar functions circle functions and pi times.
 */
class Bif_F12_CIRCLE : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_CIRCLE() : SkalarFunction(TOK_F12_CIRCLE) {}

   static Bif_F12_CIRCLE    fun;      ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_pi_times); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_circle_fun); }
};
//-----------------------------------------------------------------------------
/** Skalar functions minimum and round up.
 */
class Bif_F12_RND_UP : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_RND_UP() : SkalarFunction(TOK_F12_RND_UP) {}

   static Bif_F12_RND_UP    fun;      ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_ceiling); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_maximum); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Min_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_maximum); }
};
//-----------------------------------------------------------------------------
/** Skalar functions maximum and round down.
 */
class Bif_F12_RND_DN : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_RND_DN() : SkalarFunction(TOK_F12_RND_DN) {}

   static Bif_F12_RND_DN    fun;      ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_floor); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_minimum); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_skalar_identity_fun(B, axis, Value::Max_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_minimum); }
};
//-----------------------------------------------------------------------------
/** Skalar functions residue and magnitude.
 */
class Bif_F12_STILE : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_STILE() : SkalarFunction(TOK_F12_STILE) {}

   static Bif_F12_STILE     fun;       ///< Built-in function.

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_residue); }

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_magnitude); }

   /// Overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
   { return eval_skalar_identity_fun(B, axis, Value::Zero_P); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_residue); }
};
//-----------------------------------------------------------------------------
/** Skalar functions logarithms.
 */
class Bif_F12_LOGA : public SkalarFunction
{
public:
   /// Constructor.
   Bif_F12_LOGA() : SkalarFunction(TOK_F12_LOGA) {}

   static Bif_F12_LOGA      fun;        ///< Built-in function.

protected:
   /// Overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_skalar_B(B, &Cell::bif_nat_log); }

   /// Overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_skalar_AB(A, B, &Cell::bif_logarithm); }

   /// Overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_skalar_AXB(A, X, B, &Cell::bif_logarithm); }
};
//-----------------------------------------------------------------------------

#endif // __SKALAR_FUNCTION_HH_DEFINED__
