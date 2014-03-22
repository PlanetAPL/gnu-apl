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


#include "CollatingCache.hh"
#include "Assert.hh"
#include "Cell.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
bool
CollatingCache::greater_vec(const Cell * ca, const Cell * cb,
                            bool ascending, const void * comp_arg)
{
CollatingCache & cache = *(CollatingCache *)comp_arg;
const Rank rank = cache.get_rank();

   loop(r, rank)
      {
        loop(c, cache.get_comp_len())
           {
             const APL_Integer a = ca[c].get_int_value();
             const APL_Integer b = cb[c].get_int_value();
             const int d = cache[a].compare(cache[b], ascending, rank - r - 1);

              if (d)   return d > 0;
           }
      }

   return ca > cb;
}
//-----------------------------------------------------------------------------
int
CollatingCacheEntry::compare(const CollatingCacheEntry & other,
                             bool ascending, Rank axis) const
{
   return  (ascending) ? get_pos(axis) - other.get_pos(axis)
                       : other.get_pos(axis) - get_pos(axis);
}
//-----------------------------------------------------------------------------

