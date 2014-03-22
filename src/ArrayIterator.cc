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

#include "ArrayIterator.hh"
#include "Common.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
ArrayIteratorBase::ArrayIteratorBase(const Shape & shape)
   : max_vals(shape),
     total(0),
     is_done(false)
{
   Assert(!shape.is_empty());

   // reset values to 0 0 ... 0
   loop(r, max_vals.get_rank())   values.add_shape_item(0);

   Assert(values.get_rank() == max_vals.get_rank());
}
//-----------------------------------------------------------------------------
void ArrayIterator::operator ++()
{
   if (is_done)   return;

   ++total;

   for (Rank p = max_vals.get_rank() - 1; !is_done; --p)
       {
         if (p < 0)
            {
              is_done = true;
              return;
            }

         values.increment_shape_item(p);
         if (values.get_shape_item(p) < max_vals.get_shape_item(p))   return;

         values.set_shape_item(p, 0);   // wrap around
       }

   Assert(0 && "Not reached");
}
//-----------------------------------------------------------------------------
void PermutedArrayIterator::operator ++()
{
   if(is_done)   return;

   for (Rank up = max_vals.get_rank() - 1; !is_done; --up)
       {
         if (up < 0)
            {
              is_done = true;
              return;
            }

         const Rank pp = permutation.get_shape_item(up);

         total += weight.get_shape_item(pp);

         values.increment_shape_item(pp);
         if (values.get_shape_item(pp) < max_vals.get_shape_item(pp))   return;

         total -= weight.get_shape_item(pp) * max_vals.get_shape_item(pp);
         values.set_shape_item(pp, 0);   // wrap around
       }

   Assert(0 && "Not reached");
}
//=============================================================================
