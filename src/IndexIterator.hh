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

#include <vector>

#include "Value.hh"

//-----------------------------------------------------------------------------
/*!
   An iterator for one dimension of indices. When the iterator reached its
   end, it wraps around and increments its upper IndexIterator (if any).

   This way, multi-dimensional iterators can be created.
 */
class IndexIterator
{
public:
   /// constructor: IndexIterator with weight w
   IndexIterator(ShapeItem w)
   : weight(w),
     upper(0),
     pos(0)
   {}

    virtual ~IndexIterator() {}

   /// go the the next index
   void increment();

   /// true iff this operator has reached the end of the Array cross section.
   bool done() const   { return pos == get_index_count(); }

   /// return the number of indices
   virtual ShapeItem get_index_count() const = 0;

   /// return the current index
   virtual ShapeItem get_value() const = 0;

   /// return the current index
   virtual ShapeItem get_pos(ShapeItem p) const = 0;

   /// print this iterator (for debugging purposes).
   ostream & print(ostream & out) const;

   /// return the next higher IndexIterator
   IndexIterator * get_upper() const   { return upper; };

   /// set the next higher IndexIterator
   void set_upper(IndexIterator * up)   { Assert(upper == 0);   upper = up; };

protected:
   /// the weight of this IndexIterator
   const ShapeItem weight;

   /// The next higher iterator.
   IndexIterator * upper;

   /// The current value.
   ShapeItem pos;
};
//-----------------------------------------------------------------------------
/// an IndexIterator for an elided index
class ElidedIndexIterator : public IndexIterator
{
public:
   /// constructor
   ElidedIndexIterator(ShapeItem w, ShapeItem sh)
   : IndexIterator(w),
     count(sh)
   {}

   /// get the current index.
   virtual ShapeItem get_value() const { return pos*weight; }

   /// get the index i.
   virtual ShapeItem get_pos(ShapeItem i) const
      { Assert(i < count);   return i; }

   /// get the number of indices.
   virtual ShapeItem get_index_count() const   { return count; }

protected:
   /// number of index elements
   ShapeItem count;
};
//-----------------------------------------------------------------------------
/// an IndexIterator for a true (i.e. non-elided) index vector
class TrueIndexIterator : public IndexIterator
{
public:
   /// constructor
   TrueIndexIterator(ShapeItem w, Value_P value,
                     uint32_t qio, APL_Float qct, ShapeItem max_idx);

   /// return the current index
   virtual ShapeItem get_value() const
      { Assert(pos < indices.size());   return indices[pos]; }

   /// return the i'th index
   virtual ShapeItem get_pos(ShapeItem i) const
      { Assert(i < indices.size());   return indices[i]; }

   /// return the number of indices
   virtual ShapeItem get_index_count() const
      { return indices.size(); }

protected:
   /// the indices
   vector<ShapeItem> indices;
};
//-----------------------------------------------------------------------------
/**
    a multi-dimensional index iterator, consisting of several
   one-dimensional iterators.
 **/
class MultiIndexIterator
{
public:
   /// constructor from IndexExpr for value with rank/shape
   MultiIndexIterator(const Shape & shape, const IndexExpr & IDX);

   /// destructor
   ~MultiIndexIterator();

   /// get the next index (offset into a ravel) and increment iterators
   ShapeItem next();

   /// return true if the end of indices was reached.
   bool done() const;

protected:
   /// the iterator for the highest dimension
   IndexIterator * last_it;

   /// the iterator for the lowest dimension
   IndexIterator * lowest_it;
};
//-----------------------------------------------------------------------------

