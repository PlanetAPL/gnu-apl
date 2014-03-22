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

#ifndef __ARRAY_ITERATOR_HH_DEFINED__
#define __ARRAY_ITERATOR_HH_DEFINED__

#include "Common.hh"
#include "Value.hh"
#include "SystemLimits.hh"

//-----------------------------------------------------------------------------
/// An iterator for arrays. The iterator produces all index values of a Shape.
class ArrayIteratorBase
{
public:
   /// Contruct an iterator for an array with rank rk and shape sh.
   ArrayIteratorBase(const Shape & sh);

   /// Return true iff the iterator has reached the end of the array.
   bool done() const
      { return is_done; }

   /// Return true unless the iterator has reached the end of the array.
   bool not_done() const
      { return !is_done; }

   /// Get the max index for dimension r.
   ShapeItem get_max_value(Rank r) const
      { return max_vals.get_shape_item(r); }

   /// Get the max index for all dimensions.
   const Shape & get_max_values() const
      { return max_vals; }

   /// Get the current index for all dimensions.
   const Shape & get_values() const
      { return values; }

   /// Get the current index for dimension r.
   ShapeItem get_value(Rank r) const
      { return values.get_shape_item(r); }

   /// Get the total (the current offset into the ravel).
   ShapeItem get_total() const
      { return total; }

   /// multiply values with weight
   ShapeItem multiply(const Shape & weight)
      { Assert(values.get_rank() == weight.get_rank());
        ShapeItem ret = 0;
        loop(r, weight.get_rank())
            ret += values.get_shape_item(r) * weight.get_shape_item(r);
        return ret; }

protected:
   /// The current indices.
   Shape values;

   /// The shape of the array.
   const Shape max_vals;

   /// The current total value
   ShapeItem total;

   /// true, iff the iterator has reached the end of the array.
   bool is_done;
};
//-----------------------------------------------------------------------------
/// An iterator counting 0, 1, 2, ..., but with permutated dimensions
class ArrayIterator : public ArrayIteratorBase
{
public:
   /// constructor
   ArrayIterator(const Shape & shape)
   : ArrayIteratorBase(shape)
   {}

   /// increment the iterator
   void operator ++();
};
//-----------------------------------------------------------------------------
/** An iterator counting 0, 1, 2, ... but with permuted dimensions
    The permutation is given as a \b Shape. If perm = 0, 1, 2, ... then
    PermutedArrayIterator is the same as ArrayIterator.
 **/
class PermutedArrayIterator : public ArrayIteratorBase
{
public:
   /// constructor
   PermutedArrayIterator(const Shape & shape, const Shape & perm)
   : ArrayIteratorBase(shape),
     permutation(perm),
     weight(shape.reverse_scan())
   { Assert(shape.get_rank() == perm.get_rank()); }

   /// Increment the iterator.
   void operator ++();

protected:
   /// The current indices.
   const Shape permutation;

   /// the weights of the indices
   const Shape weight;
};
//=============================================================================

#endif // __ARRAY_ITERATOR_HH_DEFINED__
