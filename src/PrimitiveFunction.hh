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

#ifndef __SYSTEM_FUNCTION_HH_DEFINED__
#define __SYSTEM_FUNCTION_HH_DEFINED__

#include <vector>

#include "Common.hh"
#include "Function.hh"
#include "Value.hh"
#include "Id.hh"

class ArrayIterator;
class CharCell;
class IntCell;
class CollatingCache;

//-----------------------------------------------------------------------------
/**
    Base class for the APL system functions (QUAD functions and primitives
    like +, -, ...) and operators

    The individual system functions are derived from this class
 */
class PrimitiveFunction : public Function
{
public:
   /// Construct a PrimitiveFunction with \b TokenTag \b tag
   PrimitiveFunction(TokenTag tag)
   : Function(tag)
   {}

   /// Overloaded Function::has_result()
   virtual int has_result() const   { return 1; }

protected:
   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// Print the name of \b this PrimitiveFunction to \b out
   virtual ostream & print(ostream & out) const;

   /// a cell containing ' '
   static const CharCell c_filler;

   /// a cell containing 0
   static const IntCell n_filler;
};
//-----------------------------------------------------------------------------
/** The various non-skalar functions
 */
class NonskalarFunction : public PrimitiveFunction
{
public:
   /// Constructor
   NonskalarFunction(TokenTag tag)
   : PrimitiveFunction(tag)
   {}
};
//-----------------------------------------------------------------------------
/** System function zilde (⍬)
 */
class Bif_F0_ZILDE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F0_ZILDE()
   : NonskalarFunction(TOK_F0_ZILDE)
   {}

   static Bif_F0_ZILDE fun;   ///< Built-in function

   /// overladed Function::eval_()
   virtual Token eval_();

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return false; }
};
//-----------------------------------------------------------------------------
/** System function execute
 */
class Bif_F1_EXECUTE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F1_EXECUTE()
   : NonskalarFunction(TOK_F1_EXECUTE)
   {}

   static Bif_F1_EXECUTE    fun;   ///< Built-in function

   /// execute string
   static Token execute_statement(const UCS_string & statement);

   /// overladed Function::eval_B()
   virtual Token eval_B(Value_P B);

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return true; }
};
//-----------------------------------------------------------------------------
/** System function index (⌷)
 */
class Bif_F2_INDEX : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F2_INDEX()
   : NonskalarFunction(TOK_F2_INDEX)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F2_INDEX      fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions partition and enclose
 */
class Bif_F12_PARTITION : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_PARTITION()
   : NonskalarFunction(TOK_F12_PARTITION)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return partition(A, B, B->get_rank() - 1); }

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_PARTITION fun;   ///< Built-in function

protected:
   /// enclose B
   Token enclose(Value_P B);

   /// enclose B
   Token enclose_with_axis(Value_P B, Value_P X);

   /// Partition B according to A
   Token partition(Value_P A, Value_P B, Axis axis);

   /// Copy one partition to dest
   static void copy_segment(Cell * dest, ShapeItem h,
                            ShapeItem from, ShapeItem to,
                            ShapeItem m_len, ShapeItem l, ShapeItem len_l,
                            Value_P B);
};
//-----------------------------------------------------------------------------
/** primitive functions pick and disclose
 */
class Bif_F12_PICK : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_PICK()
   : NonskalarFunction(TOK_F12_PICK)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   static Bif_F12_PICK fun;   ///< Built-in function

protected:
   /// the shape of the items being disclosed
   Shape item_shape(Value_P B);

   /// Pick from B according to cA and len_A
   static Value_P pick(const Cell * cA, ShapeItem len_A, Value_P B, 
                       APL_Float qct, APL_Integer qio, bool lval);
};
//-----------------------------------------------------------------------------
/** Comma related functions (catenate, laminate, and ravel.)
 */
class Bif_COMMA : public NonskalarFunction
{
public:
   /// Constructor
   Bif_COMMA(TokenTag tag)
   : NonskalarFunction(tag)
   {}

   /// ravel along axis, with axis being the first (⍪( or last (,) axis of B
   Token ravel_axis(Value_P X, Value_P B, Axis axis);

   /// Return the ravel of B as APL value
   static Token ravel(const Shape & new_shape, Value_P B);

   /// Catenate A and B
   static Token catenate(Value_P A, Axis axis, Value_P B);

   /// Laminate A and B
   static Token laminate(Value_P A, Axis axis, Value_P B);

   /// Prepend skalar cell_A to B along axis
   static Value_P prepend_skalar(const Cell & cell_A, Axis axis, Value_P B);

   /// Prepend skalar cell_B to A along axis
   static Value_P append_skalar(Value_P A, Axis axis, const Cell & cell_B);
};
//-----------------------------------------------------------------------------
/** primitive functions catenate, laminate, and ravel along last axis
 */
class Bif_F12_COMMA : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA()
   : Bif_COMMA(TOK_F12_COMMA)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B)
      { return ravel_axis(X, B, B->get_rank()); }

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_COMMA fun;   ///< Built-in function

protected:
};
//-----------------------------------------------------------------------------
/** primitive functions catenate and laminate along first axis, table
 */
class Bif_F12_COMMA1 : public Bif_COMMA
{
public:
   /// Constructor
   Bif_F12_COMMA1()
   : Bif_COMMA(TOK_F12_COMMA1)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B)
      { return ravel_axis(X, B, 0); }

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_COMMA1    fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions take and first
 */
class Bif_F12_TAKE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_TAKE()
   : NonskalarFunction(TOK_F12_TAKE)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Take from B according to ravel_A
   static Token do_take(const Shape shape_Zi, Value_P B);

   /// Fill Z with B, pad as necessary
   static void fill(const Shape & shape_Zi, Cell * Z, Value_P B);

   static Bif_F12_TAKE      fun;   ///< Built-in function

protected:
   /// Take A from B
   Token take(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** System function drop
 */
class Bif_F12_DROP : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_DROP()
   : NonskalarFunction(TOK_F12_DROP)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_DROP      fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions member and enlist
 */
class Bif_F12_ELEMENT : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_ELEMENT()
   : NonskalarFunction(TOK_F12_ELEMENT)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_ELEMENT   fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions match and depth
 */
class Bif_F12_EQUIV : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_EQUIV()
   : NonskalarFunction(TOK_F12_EQUIV)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_EQUIV     fun;   ///< Built-in function

protected:
   /// return the depth of B
   Token depth(Value_P B);

   /// return 1 if A === B
   Token match(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** System function encode
 */
class Bif_F12_ENCODE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_ENCODE()
   : NonskalarFunction(TOK_F12_ENCODE)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_ENCODE fun;   ///< Built-in function
protected:
   /// Encode B according to A
   void encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
               const Cell * cA, const Cell & cB, double qct);
};
//-----------------------------------------------------------------------------
/** System function decode
 */
class Bif_F12_DECODE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_DECODE()
   : NonskalarFunction(TOK_F12_DECODE)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_DECODE    fun;   ///< Built-in function
protected:
   /// Decode B according to Len_A and cA
   void decode(Cell * cZ, ShapeItem len_A, const Cell * cA, ShapeItem len_B,
               const Cell * cB, ShapeItem dB, double qct);
};
//-----------------------------------------------------------------------------
/** primitive functions matrix divide and matrix invert
 */
class Bif_F12_DOMINO : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_DOMINO()
   : NonskalarFunction(TOK_F12_DOMINO)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_DOMINO    fun;   ///< Built-in function

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

protected:
   /// Invert matrix B
   Token matrix_inverse(Value_P B);

   /// Divide matrix A by matrix B
   Token matrix_divide(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse
 */
class Bif_ROTATE : public NonskalarFunction
{
public:
   /// Constructor.
   Bif_ROTATE(TokenTag tag)
   : NonskalarFunction(tag)
   {}

protected:
   /// Rotate B according to A along axis
   static Token rotate(Value_P A, Value_P B, Axis axis);

   /// Reverse B along axis
   static Token reverse(Value_P B, Axis axis);
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse along last axis
 */
class Bif_F12_ROTATE : public Bif_ROTATE
{
public:
   /// Constructor
   Bif_F12_ROTATE()
   : Bif_ROTATE(TOK_F12_ROTATE)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return reverse(B, B->get_rank() - 1); }

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return rotate(A, B, B->get_rank() - 1); }

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_ROTATE fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** primitive functions rotate and reverse along first axis
 */
class Bif_F12_ROTATE1 : public Bif_ROTATE
{
public:
   /// Constructor
   Bif_F12_ROTATE1()
   : Bif_ROTATE(TOK_F12_ROTATE1)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return reverse(B, 0); }

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return rotate(A, B, 0); }

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   static Bif_F12_ROTATE1   fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function transpose
 */
class Bif_F12_TRANSPOSE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_TRANSPOSE()
   : NonskalarFunction(TOK_F12_TRANSPOSE)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_TRANSPOSE fun;   ///< Built-in function

protected:
   /// Transpose B according to A (without diagonals)
   Value_P transpose(const Shape & A, Value_P B);

   /// Transpose B according to A (with diagonals)
   Value_P transpose_diag(const Shape & A, Value_P B);

   /// for \b sh being a permutation of 0, 1, ... rank - 1,
   /// return the inverse permutation sh⁻¹
   static Shape inverse_permutation(const Shape & sh);

   /// return sh permuted according to permutation perm
   static Shape permute(const Shape & sh, const Shape & perm);

   /// return true iff sh is a permutation
   static bool is_permutation(const Shape & sh);
};
//-----------------------------------------------------------------------------
/** primitive functions grade up and grade down
 */
class Bif_SORT : public NonskalarFunction
{
public:
   /// Constructor
   Bif_SORT(TokenTag tag)
   : NonskalarFunction(tag)
   {}

protected:
   /// a helper structure for sorting: a cahr and a shape
   struct char_shape
      {
         APL_Char achar;    ///< a char
         Shape    ashape;   ///< a shape
      };

   /// sort integer vector B 
   Token sort(Value_P B, bool ascending);

   /// sort char vector B according to collationg sequence A
   Token sort_collating(Value_P A, Value_P B, bool ascending);

   /// the collating cache that determines the order of elements
   static ShapeItem collating_cache(Unicode uni, Value_P A,
                                    CollatingCache & cache);
};
//-----------------------------------------------------------------------------
/** System function grade up
 */
class Bif_F12_SORT_ASC : public Bif_SORT
{
public:
   /// Constructor
   Bif_F12_SORT_ASC()
   : Bif_SORT(TOK_F12_SORT_ASC)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return sort(B, true); }

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return sort_collating(A, B, true); }

   static Bif_F12_SORT_ASC  fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function grade down
 */
class Bif_F12_SORT_DES : public Bif_SORT
{
public:
   /// Constructor
   Bif_F12_SORT_DES()
   : Bif_SORT(TOK_F12_SORT_DES)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return sort(B, false); }

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return sort_collating(A, B, false); }

   static Bif_F12_SORT_DES  fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** A helper struct for Bif_F12_FORMAT
 It represents a sub-field (int part, fract part, or exponent)
 of the format field
 */
struct Format_sub
{
   Format_sub()
   : out_len(0),
     flt_mask(0),
     min_len(0)
   {}

   UCS_string      format;         ///< the format ('0'...'9' and comma)
   int             out_len;        ///< the length in the output
   uint32_t        flt_mask;       ///< decorator floating mode
   int             min_len;        ///< the minimum length in the output

   /// return the number of characters in \b format
   size_t size() const
      { return format.size(); }

   /// set flt_mask according to the digits in member \b format
   /// return the number of digits from '1' to '4' (including)
   int map_field(int type);

   /// a debug function for printing \b this Format_sub
   ostream & print(ostream & out) const;

   /// return true iff format contains a '4' (i.e. counteract the effect of 
   /// 1, 2, or 3) or none of 1 (float if negative), 2 (float if positive),
   /// or 3 (always float) was given
   bool no_float() const
        { if (flt_mask & BIT_4)   return true;        // counteract 1, 2, or 3
          return ! (flt_mask & (BIT_1 | BIT_2 | BIT_3));   // no 1, 2, or 3
        }

   /// return true if the decorator is floating floating 
   bool do_float(bool value_is_negative) const
      { if (flt_mask & BIT_1)   return  value_is_negative;
        if (flt_mask & BIT_2)   return !value_is_negative;
        return true;
      }

   /// return the pad character (default: space, otherwise controlled by ⎕FC)
   Unicode pad_char(Unicode qfc) const
      { return flt_mask & BIT_8 ? qfc : UNI_ASCII_SPACE; }

   /// Fill buf at position x,y with data according to fmt
   UCS_string insert_int_commas(const UCS_string & data,
                                bool & overflow) const;

   /// Fill buf at position x,y with data according to fmt
   UCS_string insert_fract_commas(const UCS_string & data) const;
};
//-----------------------------------------------------------------------------
/** System function format
 */
class Bif_F12_FORMAT : public NonskalarFunction
{
public:
   /// constructor
   Bif_F12_FORMAT()
   : NonskalarFunction(TOK_F12_FORMAT)
   {}

   static Bif_F12_FORMAT    fun;   ///< Built-in function

   /// Return true iff uni is '0' .. '9', comma, or full-stop
   static bool is_control_char(Unicode uni);

   /// An entire format field (LIFER = Left-Int-Fract-Expo-Right
   struct Format_LIFER
      {
        /// constructor
        Format_LIFER(UCS_string fmt);

        /// return the size of the format
        size_t format_size() const
           { return left_deco.size() + int_part.size() + fract_part.size()
                  + expo_deco.size() + exponent.size() + right_deco.size(); }

        /// return the size of the output
        size_t out_size() const
           { return left_deco.out_len + int_part.out_len + fract_part.out_len
                  + expo_deco.out_len + exponent.out_len + right_deco.out_len; }

        /// format \b value by example
        UCS_string format_example(APL_Float value);

        /// format the left decorator and integer part (everything left of the
        /// decimal dot)
        UCS_string format_left_side(const UCS_string data_int, bool negative,
                                    bool & overflow);

        /// format the fract part, exponent, and right decorator (everything
        /// left of the decimal dot)
        UCS_string format_right_side(const UCS_string data_fract, bool negative,
                                     const UCS_string data_expo);

        /// Print \b value into int, fract, and expo fields
        void fill_data_fields(double value, UCS_string & data_int,
                              UCS_string & data_fract, UCS_string & data_expo);

        Format_sub left_deco;    ///< the left decorator
        Format_sub int_part;     ///< the integer part
        Format_sub fract_part;   ///< the fractional part
        Format_sub expo_deco;    ///< the exponent decorator
        Format_sub exponent;     ///< the exponent
        Format_sub right_deco;   ///< the left decorator

        /// the exponent char to be used
        Unicode exponent_char;

        /// true if the exponent is negative
        bool expo_negative;
      };

   /// A character array with the display of B
   static Value_P monadic_format(Value_P B);

protected:
   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// A character array with B formatted by example
   Value_P format_by_example(Value_P A, Value_P B);

   /// split entire format string string into \b column format strings
   void split_example_into_columns(const UCS_string & format,
                                   vector<UCS_string> & col_formats);

   /// A character array with  a columns of B formatted by specification
   PrintBuffer format_col_spec(int width, int precision, const Cell * cB,
                               int cols, int rows);

   /// add a row (consisting of \b data) to \b PrintBuffer \b ret
   void add_row(PrintBuffer & ret, int row, bool has_char, bool has_num,
                Unicode align_char, UCS_string & data);

   /// A character array with B formatted by specification
   Value_P format_by_specification(Value_P A, Value_P B);

   /// format value with \b precision mantissa digits (floating format)
   static UCS_string format_spec_float(APL_Float value, int precision);

   /// format value with \b precision mantissa digits (exponential format)
   static UCS_string format_spec_expo(APL_Float value, int precision);
};
//-----------------------------------------------------------------------------
/** System function index of (⍳)
 */
class Bif_F12_INDEX_OF : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_INDEX_OF()
   : NonskalarFunction(TOK_F12_INDEX_OF)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   static Bif_F12_INDEX_OF fun;   ///< Built-in function

protected:
};
//-----------------------------------------------------------------------------
/** primitive functions reshape and shape
 */
class Bif_F12_RHO : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F12_RHO()
   : NonskalarFunction(TOK_F12_RHO)
   {}

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Reshape B according to rank and shape
   static Token do_reshape(const Shape & shape, Value_P B);

   static Bif_F12_RHO fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function unique
 */
class Bif_F1_UNIQUE : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F1_UNIQUE()
   : NonskalarFunction(TOK_F1_UNIQUE)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_B(Value_P B);

   static Bif_F1_UNIQUE      fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function left (⊣)
 */
class Bif_F2_LEFT : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F2_LEFT()
   : NonskalarFunction(TOK_F2_LEFT)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return Token(TOK_APL_VALUE1, A->clone(LOC)); }

   static Bif_F2_LEFT fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function right (⊢)
 */
class Bif_F2_RIGHT : public NonskalarFunction
{
public:
   /// Constructor
   Bif_F2_RIGHT()
   : NonskalarFunction(TOK_F2_RIGHT)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return Token(TOK_APL_VALUE1, B->clone(LOC)); }

   static Bif_F2_RIGHT fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------

#endif // __SYSTEM_FUNCTION_HH_DEFINED__
