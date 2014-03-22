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

#include "Error.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "Output.hh"
#include "PrintOperator.hh"

//-----------------------------------------------------------------------------
void
IndexIterator::increment()
{
   ++pos;
   if (pos >= get_index_count())
      {
         if (upper)
            {
              pos = 0;
              upper->increment();
            }
         else
            {
              pos = get_index_count();   // so that done() works
            }
      }

   // print(CERR);
}
//-----------------------------------------------------------------------------
ostream &
IndexIterator::print(ostream & out) const
{
   out << "Iterator "        << HEX(this) << ":"  << endl
       << "   upper:       " << HEX(upper)        << endl
       << "   value:       " << get_value()       << endl
       << "   indices[" << get_index_count() << "] :";

   loop(i, get_index_count())   out << " " << get_pos(i);

   return out << endl;
}
//=============================================================================
TrueIndexIterator::TrueIndexIterator(ShapeItem w, Value_P value,
                                     uint32_t qio, APL_Float qct,
                                     ShapeItem max_idx)
   : IndexIterator(w)
{
// CERR << "TrueIndexIterator(w=" << w << ", value=" << *value
//      << ", max=" << max_idx << ")" << endl;

const ShapeItem vlen = value->element_count();

   indices.reserve(vlen);
   loop(v, vlen)
      {
        const ShapeItem idx = value->get_ravel(v).get_near_int(qct) - qio;

        // instead of testing signed < 0 and >= max, we test unsigned >= max.
        //
        if (idx >= max_idx)   INDEX_ERROR;

        indices.push_back(idx*w);
      }
}
//=============================================================================
MultiIndexIterator::MultiIndexIterator(const Shape & shape,
                                       const IndexExpr & IDX)
   : last_it(0),
     lowest_it(0)
{
   // IDX is parsed from right to left:    Value[I2;I1;I0]  --> I0 I1 I2
   // the shapes of Value and IDX are then related as follows:
   //
   // Value    IDX
   // ---------------
   // 0        rank-1   (rank = IDX->value_count())
   // 1        rank-2
   // ...      ...
   // rank-2   1
   // rank-1   0
   // ---------------
   //

   if (shape.get_rank() != IDX.value_count())
      {
        Log(LOG_error_throw)
           {
             Q(shape.get_rank())
             Q(shape)
             Q(IDX)
             Q(IDX.value_count())
           }
        INDEX_ERROR;
      }

ShapeItem weight = 1;
   loop(idx_r, shape.get_rank())
       {
         const Rank val_r = shape.get_rank() - idx_r - 1;  // see comment above.
         const ShapeItem  sh_r = shape.get_shape_item(val_r);
         Value_P I = IDX.values[idx_r];

         IndexIterator * new_it = !!I
           ? (IndexIterator *)(new TrueIndexIterator(weight, I, IDX.quad_io,
                                                    IDX.quad_ct, sh_r))
           : (IndexIterator *)(new ElidedIndexIterator(weight, sh_r));

         Log(LOG_delete)
            CERR << "new    " << (const void *)new_it << " at " LOC << endl;

         if (last_it)   last_it->set_upper(new_it);
         else           lowest_it = new_it;

         weight *= sh_r;
         last_it = new_it;
       }

   Log(LOG_delete)   CERR << "delete " HEX(&IDX) << " at " LOC << endl;
   delete &IDX;
}
//-----------------------------------------------------------------------------
MultiIndexIterator::~MultiIndexIterator()
{
   for (IndexIterator * it = lowest_it; it;)
       {
         IndexIterator * del = it;
         it = it->get_upper();
         Log(LOG_delete)   CERR << "delete " HEX(del) << " at " LOC << endl;
         delete del;
       }
}
//-----------------------------------------------------------------------------
ShapeItem
MultiIndexIterator::next()
{
   Assert(!lowest_it || !done());

ShapeItem ret = 0;
   for (IndexIterator * it = lowest_it; it; it = it->get_upper())
            ret += it->get_value();

   if (lowest_it)   lowest_it->increment();
   return ret;
}
//-----------------------------------------------------------------------------
bool
MultiIndexIterator::done() const
{
   if (last_it == 0)   return true;
   return last_it->done();
}
//=============================================================================

