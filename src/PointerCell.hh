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

#ifndef __POINTERCELL_HH_DEFINED__
#define __POINTERCELL_HH_DEFINED__

#include "Cell.hh"

//-----------------------------------------------------------------------------
/*!
    A cell pointing to another APL value. This is used to create nested
    arrays. This class essentially overloads certain functions in class
    Cell with nested array specific implementations.
 */
class PointerCell : public Cell
{
public:
   /// Construct an cell containing nested sub-array \b val.
   PointerCell(Value_P val);

   /// Overloaded Cell::is_pointer_cell().
   virtual bool is_pointer_cell()   const   { return true; }

   /// Overloaded Cell::get_pointer_value().
   virtual Value_P get_pointer_value()  const;

   /// Overloaded Cell::greater().
   virtual bool greater(const Cell * other, bool ascending) const;

   /// Overloaded Cell::equal().
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// Overloaded Cell::release().
   virtual void release(const char * loc);

   /// the Quad_CR representation of this cell.
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

protected:
   ///  Overloaded Cell::get_cell_type()
   virtual CellType get_cell_type() const
      { return CT_POINTER; }

   ///  Overloaded Cell::compute_cell_types()
   virtual CellType compute_cell_types() const;

   /// Overloaded Cell::get_classname().
   virtual const char * get_classname() const   { return "PointerCell"; }

   /// Overloaded Cell::CDR_size() should not be called for pointer cells
   virtual int CDR_size() const { NeverReach("CDR_size called on PointerCell base class"); }
};
//=============================================================================

#endif // __POINTERCELL_HH_DEFINED__
