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

#ifndef __INDEXEXPR_HH_DEFINED__
#define __INDEXEXPR_HH_DEFINED__

#include <vector>

#include "Cell.hh"
#include "DynamicObject.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
/**
     An array of index values.
 */
class IndexExpr : public DynamicObject
{
public:
   /// constructor: empty (0-dimensional) IndexExpr
   IndexExpr(bool _left, const char * loc);

   /// The quad-ct for this index.
   APL_Float quad_ct;

   /// The quad-io for this index.
   uint32_t quad_io;

   /// The values (0 = elided index as in [] or [;].
   Value_P values[MAX_RANK];

   /// append a value.
   void add(Value_P val)
      { Assert(rank < MAX_RANK);
        values[rank++] = val; }

   /// return the number of values (= number of semicolons + 1)
   size_t value_count() const   { return rank; }

   /// return true iff the number of dimensions is 1 (i.e. no ; and non-empty)
   bool is_axis() const   { return rank == 1; }

   /// Return an axis (from an IndexExpr of rank 1.
   Rank get_axis(Rank max_axis) const;

   /// set axis \b axis to \b val
   void set_value(Axis axis, Value_P val);

   /// return true iff this index is part of indexed assignment ( A[]← )
   bool get_left() const
      { return left; }

   /// Return a set of axes as shape. This is used to convert a number of
   /// axes (like in [2 3]) into a shape (like 2 3).
   Shape to_shape() const;

   /// return one axis value and clear it in \b this IndexExpr
   Value_P extract_value(Rank rk);

   /// print stale IndexExprs, and return the number of stale IndexExprs.
   static int print_stale(ostream & out);

   /// erase stale IndexExprs
   static void erase_stale(const char * loc);

protected:
   /// the number of dimensions
   Rank rank;

   /// true iff this index is part of indexed assignment ( A[]← )
   const bool left;
};
//-----------------------------------------------------------------------------

#endif // __INDEXEXPR_HH_DEFINED__
