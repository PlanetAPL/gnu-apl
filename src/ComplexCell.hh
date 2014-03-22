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

#ifndef __COMPLEXCELL_HH_DEFINED__
#define __COMPLEXCELL_HH_DEFINED__

#include "NumericCell.hh"

//-----------------------------------------------------------------------------
/*!
 A cell containing a single APL complex value. This class essentially
 overloads certain functions in class Cell with complex number specific
 implementations.
 */
class ComplexCell : public NumericCell
{
public:
   /// Construct an complex number cell from a complex number
   ComplexCell(APL_Complex c);

   /// Construct an complex number cell from real part \b r and imag part \b i.
   ComplexCell(APL_Float r, APL_Float i);

   /// Destructor.
   ~ComplexCell();

   /// Overloaded Cell::is_complex_cell().
   virtual bool is_complex_cell() const   { return true; }

   /// Overloaded Cell::greater().
   virtual bool greater(const Cell * other, bool ascending) const;

   /// Overloaded Cell::equal().
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_ceiling(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_conjugate(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_direction(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_exponential(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_factorial(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_floor(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_magnitude(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_nat_log(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_negative(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_pi_times(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_reciprocal(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_roll(Cell * Z) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_add(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_subtract(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_divide(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_equal(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_logarithm(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_multiply(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_power(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_residue(Cell * Z, const Cell * A) const;

   /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual void bif_circle_fun(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::release()
   virtual void release(const char * loc);

   /// the Quad_CR representation of this cell.
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

   /// return true iff this cell needs scaling (exponential format) in pctx.
   virtual bool need_scaling(const PrintContext &pctx) const;

   /// the lanczos approximation for gamma(1.0 + x + iy) for x >= 0.
   static APL_Complex gamma(APL_Float x, const APL_Float y);

protected:
   /// 1.0
   static APL_Complex ONE()       { return APL_Complex(1, 0); }

   /// i
   static APL_Complex PLUS_i()   { return APL_Complex(0, 1); }

   /// -i
   static APL_Complex MINUS_i()   { return APL_Complex(0, -1); }

   /// the phase of \b cpx number.
   APL_Float phase() const;

   /// Overloaded Cell::get_cell_type().
   virtual CellType get_cell_type() const
      { return CT_COMPLEX; }

   /// Overloaded Cell::get_real_value().
   virtual APL_Float get_real_value() const;

   /// Overloaded Cell::get_imag_value().
   virtual APL_Float get_imag_value() const;

   /// Overloaded Cell::get_complex_value()
   virtual APL_Complex get_complex_value() const   { return *value.cpxp; }

   /// Overloaded Cell::get_near_bool().
   virtual bool get_near_bool(APL_Float qct)  const;

   /// Overloaded Cell::get_near_int().
   virtual APL_Integer get_near_int(APL_Float qct)  const;

   /// Overloaded Cell::get_checked_near_int().
   virtual APL_Integer get_checked_near_int()  const
      { return APL_Integer(value.cpxp->real() + 0.3); }

   /// Overloaded Cell::is_near_int().
   virtual bool is_near_int(APL_Float qct) const;

   /// Overloaded Cell::is_near_zero().
   virtual bool is_near_zero(APL_Float qct) const;

   /// Overloaded Cell::is_near_one().
   virtual bool is_near_one(APL_Float qct) const;

   /// Overloaded Cell::is_near_real().
   virtual bool is_near_real(APL_Float qct) const
      { return (value.cpxp->imag() < qct) && (value.cpxp->imag() > -qct); }

   /// Overloaded Cell::get_classname().
   virtual const char * get_classname() const   { return "ComplexCell"; }

   /// Overloaded Cell::demote_complex_to_real().
   virtual void demote_complex_to_real(APL_Float qct);

   /// Overloaded Cell::CDR_size()
   virtual int CDR_size() const { return 16; }
};
//=============================================================================

#endif // __COMPLEXCELL_HH_DEFINED__
