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

#ifndef __REALCELL_HH_DEFINED__
#define __REALCELL_HH_DEFINED__

#include "NumericCell.hh"

/*! A cell that is either an integer cell, a floating point cell.
    This class contains all cell functions for which the detailed type
    makes no difference.
*/

class RealCell : public NumericCell
{
protected:
   /// Overloaded Cell::is_real_cell().
   virtual bool is_real_cell() const { return true; }

   /// Overloaded Cell::bif_add().
   virtual void bif_add(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_circle_fun().
   virtual void bif_circle_fun(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_divide().
   virtual void bif_divide(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_logarithm().
   virtual void bif_logarithm(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_multiply().
   virtual void bif_multiply(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_power().
   virtual void bif_power(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::bif_subtract().
   virtual void bif_subtract(Cell * Z, const Cell * A) const;

   /// Overloaded Cell::get_classname().
   virtual const char * get_classname() const   { return "RealCell"; }
};
//-----------------------------------------------------------------------------

#endif // __REALCELL_HH_DEFINED__
