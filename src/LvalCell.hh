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

#ifndef __LVALCELL_HH_DEFINED__
#define __LVALCELL_HH_DEFINED__

#include "Cell.hh"

//-----------------------------------------------------------------------------
/**
    A cell pointing to another cell.
    This is used for selective assignments.
 **/
class LvalCell : public Cell
{
public:
   /// Construct an cell pointing to another cell
   LvalCell(Cell * cell)
      { value.lval = cell; }

   /// Overloaded Cell::is_lval_cell().
   virtual bool is_lval_cell()  const   { return true; }

   /// Overloaded Cell::get_lval_value().
   virtual Cell * get_lval_value() const;

   /// Overloaded Cell::character_representation().
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

protected:
   ///  Overloaded Cell::get_cell_type()
   virtual CellType get_cell_type() const
      { return CT_CELLREF; }

   /// Overloaded Cell::get_classname().
   virtual const char * get_classname() const   { return "LvalCell"; }

  /// Overloaded Cell::CDR_size() should not be called for lval cells
   virtual int CDR_size() const { NeverReach("CDR_size called on LvalCell base class"); }
};
//=============================================================================

#endif // __LVALCELL_HH_DEFINED__
