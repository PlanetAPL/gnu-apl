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

#ifndef __CELL_HH_DEFINED__
#define __CELL_HH_DEFINED__

#include <complex>

#include "Parser.hh"
#include "PrintBuffer.hh"
#include "SystemLimits.hh"

class Value;

//-----------------------------------------------------------------------------
/**
 **   Base class for one item of an APL ravel. The item is one of the following:
 **
 **  A character skalar
 **  An integer skalar
 **  A floating point skalar
 **  A complex number skalar
 **  An APL array
 **
 **   The kind of item is defined by its class, which is derived from Cell.
 **/
class Cell
{
public:
   /// Construct a Cell
   Cell() {}

   /// Constructor from other cell. This should only be used by
   /// vector<Cell>::resize(). resize() copies Cell() into the new Cells,
   /// therefore the cell type of the new cells is expected to be CT_BASE.
   Cell(const Cell & other)
      { Assert(other.get_cell_type() == CT_BASE); }

   /// deep copy of cell \b other into \b this cell
   void init(const Cell & other);

   /// init this Cell from value. If value is a skalar then its first element
   /// is used (and value is erased). Otherwise a PointerCell is created.
   void init_from_value(Value_P value, const char * loc);

   /// return pointer value of a PointerCell or create a skalar with a
   /// copy of this cell.
   Value_P to_value(const char * loc) const;

   /// init \b this cell to be the type of \b other
   void init_type(const Cell & other);

   /// Return \b true if \b this cell is greater than \b other
   virtual bool greater(const Cell * other, bool ascending) const;

   /// Return \b true if \b this cell is equal to \b other
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// Return the character value of a cell
   virtual Unicode get_char_value() const   { DOMAIN_ERROR; }

   /// Return the integer value of a cell
   virtual APL_Integer get_int_value() const   { DOMAIN_ERROR; }

   /// Return the real part of a cell
   virtual APL_Float get_real_value() const   { DOMAIN_ERROR; }

   /// Return the imaginary part of a cell
   virtual APL_Float get_imag_value() const   { DOMAIN_ERROR; }

   /// Return the complex value of a cell
   virtual APL_Complex get_complex_value() const   { DOMAIN_ERROR; }

   /// Return the APL value of a cell (Asserts for non-pointer cells)
   virtual Value_P get_pointer_value()  const   { DOMAIN_ERROR; }

   /// Return the APL value of a cell (Asserts for non-lval cells)
   virtual Cell * get_lval_value() const   { LEFT_SYNTAX_ERROR; }

   /// the Quad_CR representation of this cell
   virtual PrintBuffer character_representation(const PrintContext &pctx) const
      { DOMAIN_ERROR; }

   /// return true if this cell needs scaling (exponential format) in pctx
   virtual bool need_scaling(const PrintContext &pctx) const
      { return false; }

   /// Return value if it is close to boolean, or else throw DOMAIN_ERROR
   virtual bool get_near_bool(APL_Float qct)  const
      { DOMAIN_ERROR; }

   /// Return value if it is close to int, or else throw DOMAIN_ERROR
   virtual APL_Integer get_near_int(APL_Float qct)  const
      { DOMAIN_ERROR; }

   /// Return value if it is (known to be) close to int, or else Assert()
   virtual APL_Integer get_checked_near_int()  const
      { NeverReach("Value is not an integer"); }

   /// return true iff value is numeric and close to 0
   virtual bool is_near_zero(APL_Float qct) const
      { return false; }

   /// return true iff value is numeric and close to 1
   virtual bool is_near_one(APL_Float qct) const
      { return false; }

   /// return true iff value is numeric and close to 0 or 1
   bool is_near_bool(APL_Float qct) const
      { return is_near_zero(qct) || is_near_one(qct); }

   /// True iff value is numeric and close to an int
   virtual bool is_near_int(APL_Float qct) const
      { return false; }

   /// True iff value is numeric and close to a real, or throw DOMAIN_ERROR
   virtual bool is_near_real(APL_Float qct) const
      { return false; }

   /// return the minimum number of data bytes to store this cell in
   /// CDR format. The actual number of data bytes can be bigger if other
   /// cells need more bytes,
   virtual int CDR_size() const { NeverReach("CDR_size called on Cell base class"); }

   /// Release content pointed to (complex, APL value)
   virtual void release(const char * loc) {}

   /// Return \b true iff \b this cell is an integer cell
   virtual bool is_integer_cell() const
      { return false; }

   /// Return \b true iff \b this cell is a floating point cell
   virtual bool is_float_cell() const
      { return false; }

   /// Return \b true iff \b this cell is a character cell
   virtual bool is_character_cell() const
      { return false; }

   /// Return \b true iff \b this cell is a pointer (to a sub-array) cell
   virtual bool is_pointer_cell() const
      { return false; }

   /// Return \b true iff \b this cell is a pointer to a cell
   virtual bool is_lval_cell() const
      { return false; }

   /// Return \b true iff \b this cell is a complex number cell
   virtual bool is_complex_cell() const
      { return false; }

   /// Return \b true iff \b this cell is an integer, float, or complex cell
   virtual bool is_numeric() const
      { return false; }

   /// Return \b true iff \b this cell is an integer or float cell
   virtual bool is_real_cell() const   // int or flt
      { return false; }

   /// Return \b true iff \b this cell is an example field character cell
   virtual bool is_example_field() const
      { return false; }

   /// The possible cell values
   union SomeValue
      {
        Unicode       aval;    ///< A character
        APL_Integer   ival;    ///< An integer
        APL_Float     fval;    ///< A floating point number
        APL_Complex  *cpxp;   ///< Pointer to a complex number
        void         *vptr;   ///< A void pointer
        Cell         *next;   ///< pointer to the next (unused) cell
        Cell         *lval;   ///< left value (for selective assignment)
        VALUE_P(      valp)   ///< Pointer to a nested sub-array
      };

   /// return the type of \b this cell
   virtual CellType get_cell_type() const
      { return CT_BASE; }

   /// return the name of the class
   virtual const char * get_classname()  const   { return "Cell"; }

   /// return the types of \b this cell and nested values
   virtual CellType compute_cell_types() const
      { return get_cell_type(); }

   /// convert near-real complex to real or integer
   virtual void demote_complex_to_real(APL_Float qct) {}

   /// convert near-int real to integer
   virtual void demote_float_to_int(APL_Float qct) {}

   /// round the value of this Cell up and store the result in Z
   virtual void bif_ceiling(Cell * Z) const
      { DOMAIN_ERROR; }

   /// conjugate the value of this Cell and store the result in Z
   virtual void bif_conjugate(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the direction (sign) of this Cell in Z
   virtual void bif_direction(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store e to the power of the value of this Cell in Z
   virtual void bif_exponential(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the faculty (N!) of the value of this Cell in Z
   virtual void bif_factorial(Cell * Z) const
      { DOMAIN_ERROR; }

   /// Round the value of this Cell down and store the result in Z
   virtual void bif_floor(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the absolute value of this Cell in Z
   virtual void bif_magnitude(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the base e logarithm of the value of this Cell in Z
   virtual void bif_nat_log(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the negative of the value of this Cell in Z
   virtual void bif_negative(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the logical complement of the value of this Cell in Z
   virtual void bif_not(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store pi times the value of this Cell in Z
   virtual void bif_pi_times(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the reciprocal of the value of this Cell in Z
   virtual void bif_reciprocal(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store a random number between 1 and the value of this Cell in Z
   virtual void bif_roll(Cell * Z) const
      { DOMAIN_ERROR; }

   /// store the sum of A and \b this cell in Z
   virtual void bif_add(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the logical and of A and \b this cell in Z
   virtual void bif_and(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the binomial (n over k) of A and \b this cell in Z
   virtual void bif_binomial(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the difference between A and \b this cell in Z
   virtual void bif_subtract(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the quotient between A and \b this cell in Z
   virtual void bif_divide(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store 1 in Z if A == the value of \b this cell in Z, else 0
   virtual void bif_equal(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store 1 in Z if A >= the value of \b this cell in Z, else 0
   virtual void bif_greater_than(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store 1 in Z if A < the value of \b this cell in Z, else 0
   virtual void bif_less_than(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store 1 in Z if A != the value of \b this cell in Z, else 0
   void bif_not_equal(Cell * Z, const Cell * A) const;

   /// store 1 in Z if A is >= the value of \b this cell in Z, else 0
   void bif_greater_eq(Cell * Z, const Cell * A) const;

   /// store 1 in Z if A <= the value of \b this cell in Z, else 0
   void bif_less_eq(Cell * Z, const Cell * A) const;

   /// store the base A logarithm of \b this cell in Z
   virtual void bif_logarithm(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the larger of A and of \b this cell in Z
   virtual void bif_maximum(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the smaller of A and of \b this cell in Z
   virtual void bif_minimum(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the product of A and \b this cell in Z
   virtual void bif_multiply(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the logical nand of A and \b this cell in Z
   virtual void bif_nand(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the logical nor of A and \b this cell in Z
   virtual void bif_nor(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store the logical or of A and \b this cell in Z
   virtual void bif_or(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store A to the power of \b this cell in Z
   virtual void bif_power(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store A modulo \b this cell in Z
   virtual void bif_residue(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// store a circle function (according to A) of \b this cell in Z
   virtual void bif_circle_fun(Cell * Z, const Cell * A) const
      { DOMAIN_ERROR; }

   /// placement new
   void * operator new(std::size_t, void *);

   /// copy (deep) count cells from src to dest)
   static void copy(Cell * & dst, const Cell * & src, ShapeItem count)
      { loop(c, count)   dst++->init(*src++); }

   /// copy (deep) count cells from src to val (which is under construction))
   static void copy(Value & val, const Cell * & src, ShapeItem count);

   /// true iff value is close to an int (within +- qct)
   static bool is_near_int(APL_Float value, APL_Float qct);

   /// true iff value is close to 0 (within +- qct)
   static bool is_near_zero(APL_Float value, APL_Float qct)
      { return (value < qct) && (value > -qct); }

   /// return value if it is close to int, or else throw DOMAIN_ERROR
   static APL_Integer near_int(APL_Float value, APL_Float qct);

   /// return \b ascending iff a > b
   typedef bool (*greater_fun)(const Cell * a, const Cell * b, bool ascending,
                             const void * comp_arg);
   /// Sort a[]
   static void heapsort(const Cell ** a, ShapeItem heapsize, bool ascending,
                        const void * comp_arg, greater_fun gf);

   /// compare comp_len items
   static bool greater_vec(const Cell * a, const Cell * b, bool ascending,
                           const void * comp_arg);

protected:
   /// the value of \b this cell
   SomeValue value;

   /// sort subtree starting at a[i] into a heap
   static void make_heap(const Cell ** a, ShapeItem heapsize, int64_t i,
                         bool ascending, const void * comp_arg, greater_fun gf);

   /// sort a[] into a heap
   static void init_heap(const Cell ** a, ShapeItem heapsize, bool ascending,
                         const void * comp_arg, greater_fun gf);

private:
   /// Cells that are allocated with new() shall always be contained in
   /// APL values (using placement new() on the ravel of the value.
   /// We prevent the accidental use of non-placement new() by defining
   /// it but not implementing it.
   void * operator new(std::size_t);
};
//-----------------------------------------------------------------------------

#endif // __CELL_HH_DEFINED__
