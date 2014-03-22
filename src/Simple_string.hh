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

#ifndef __SIMPLE_STRING_HH_DEFINED__
#define __SIMPLE_STRING_HH_DEFINED__

#include <iostream>

#ifdef AUXILIARY_PROCESSOR
# define __ASSERT_HH_DEFINED__
# include <assert.h>
# define Assert(x) assert(x)
#else
# include "Assert.hh"
# include "Common.hh"
#endif

#include "Backtrace.hh"

using namespace std;

//-----------------------------------------------------------------------------
/// a simple string
template <class T>
class Simple_string
{
public:
   /// constructor: empty string
   Simple_string()
   : items_allocated(MIN_ALLOC),
     items_valid(0)
      {
        items = new T[items_allocated];
      }

   /// constructor: the first \b len items of \b data
   Simple_string(const T * data, int len)
   : items_allocated(len + ADD_ALLOC),
     items_valid(len)
      {
        Assert1(items_valid >= 0);
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        _copy(items, data, len);
      }

   /// constructor: \b len times \b data
   Simple_string(int len, T data)
   : items_allocated(len + ADD_ALLOC),
     items_valid(len)
      {
        Assert(items_valid >= 0);
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        for (unsigned int l = 0; l < len; ++l)   items[l] = data;
      }

   /// constructor: copy other string
   Simple_string(const Simple_string & other)
   : items_allocated(other.items_valid + ADD_ALLOC),
     items_valid(other.items_valid)
      {
        Assert(items_valid >= 0);
        items = new T[items_allocated];
        _copy(items, other.items, items_valid);
      }

   /// constructor: copy other string, starting at pos, at most len
   Simple_string(const Simple_string & other, int pos, int len)
      {
        if (len > other.items_valid - pos)   len = other.items_valid - pos;
        if (len < 0)   len = 0;

        items_valid = len;
        items_allocated = items_valid + ADD_ALLOC;
        if (items_allocated < MIN_ALLOC)   items_allocated = MIN_ALLOC;
        items = new T[items_allocated];
        _copy(items, other.items + pos, len);
      }

   /// destructor
   ~Simple_string()
      { 
        delete [] items;
      }

   /// copy \b other
   Simple_string & operator =(const Simple_string & other)
      {
        delete [] items;
        new (this) Simple_string(other);
        return *this;
      }

   /// 0-terminate the items and return a pointer to them
   /// return the items of the string (not 0-terminated)
   const T * get_items() const
      {
        return items;
      }

   /// return true iff \b this is equal to \b other
   bool operator ==(const Simple_string<T> & other) const
      {
        if (items_valid != other.items_valid)   return false;
        for (int c = 0; c < items_valid; ++c)
            if (items[c] != other.items[c])   return false;
        return true;
      }

   /// return true iff \b this is different from \b other
   bool operator !=(const Simple_string<T> & other) const
      { return !(*this == other); }

   /// return the number of characters in \b this string
   int size() const
      { return items_valid; }

   /// return the idx'th character
   const T & operator[](unsigned int idx) const
      {
        Assert(idx < items_valid);
        return items[idx];
      }

   /// return the idx'th character
   T & operator[](unsigned int idx)
      {
        if (idx >= items_valid)
           {
             debug(cerr) << "idx = " << idx << endl;
             Assert(0 && "Bad index");
           }
        return items[idx];
      }

   /// append character \b t to \b this string
   void append(const T & t)
      {
        if (items_valid >= items_allocated)   extend();
        copy_1(items[items_valid++], t, LOC);
      }

   /// append character \b t to \b this string
   void append(const T & t, const char * loc)
      {
        if (items_valid >= items_allocated)   extend();
        copy_1(items[items_valid++], t, loc);
      }

   /// append string \b other to \b this string
   void append(const Simple_string & other)
      {
        if (other.items_valid)
           {
             extend(items_valid + other.items_valid);
             _copy(items + items_valid, other.items, other.items_valid);
             items_valid += other.items_valid;
           }
      }

   /// insert character \b t after position \b pos
   void insert_after(int pos, const T & t)
      {
        Assert((unsigned int)pos < items_valid);
        if (items_valid >= items_allocated)   extend();
        for (int s = items_valid - 1; s > pos; --s)  items[s + 1] = items[s];
        items[pos + 1] = t;
        ++items_valid;
      }

   /// insert character \b t before position \b pos
   void insert_before(int pos, const T & t)
      {
        Assert((unsigned int)pos < items_valid);
        if (items_valid >= items_allocated)   extend();
        for (int s = items_valid - 1; s >= pos; --s)   items[s + 1] = items[s];
        items[pos] = t;
        ++items_valid;
      }

   /// decrease size to \b new_size
   void shrink(unsigned int new_size)
      {
        Assert(items_valid >= new_size);
        items_valid = new_size;
      }

   /// insert \b count characters \b t after position \b pos
   void insert(int pos, int count, T t)
      {
        Assert(count >= 0);
        Assert(pos <= items_valid);
        extend(items_valid + count);
        if (pos < items_valid)
           copy_downwards(items + pos + count, items + pos, items_valid - pos);
        for (int c = 0; c < count; ++c)  items[pos + c] = t;
        items_valid += count;
      }

   /// forget last element
   void pop()
      { if (items_valid)   --items_valid; }

   const T & last() const
      { return items[items_valid - 1]; }

   T & last()
      { return items[items_valid - 1]; }

   /// shrink to size 0
   void clear()
      { items_valid = 0; }

   /// erase all items above pos
   void erase(int pos)
      {
        if (items_valid > pos)   items_valid = pos;
      }

   /// erase \b count items above pos
   void erase(int pos, int count)
      {
        if (pos >= items_valid)   return;   // nothing to erase

         // rest is the number of items right of pos.
         //
         // before:     front     erased        rest
         //             0                              
         //             <- pos -> <-count->
         //             <-  pos + count  ->     
         //
         const int rest = items_valid - (pos + count);

         if (rest < 0)   // erase more than we have, i.e. no rest
           {
             items_valid = pos;
             return;
           }

         _copy(items + pos, items + pos + count, rest);
         items_valid -= count;
      }

   /// erase first character
   void remove_front()   { erase(0, 1); }

   /// extend allocated size
   void reserve(unsigned int new_alloc_size)
      {
        extend(new_alloc_size);
      }

   /// (do not) turn T into uppercase T
   static T no_upper (T t) { return t; }

   /// compare strings
   int compare(const Simple_string & other, T (*upper)(T) = no_upper) const
      {
        const unsigned int common_len = items_valid < other.items_valid
                                      ? items_valid : other.items_valid;
        for (unsigned int c = 0; c < common_len; ++c)
            {
              if (upper(items[c]) < upper(other.items[c]))   return -1;
              if (items[c] > other.items[c])   return  1;
            }

        if (items_valid < other.items_valid)   return -1;
        if (items_valid > other.items_valid)   return  1;
        return 0;
      }

protected:
   /// print properties
   ostream & debug(ostream & out)
     {
       out << "items_allocated = " << items_allocated << endl
            << "items[" << items_valid << "] = ";
       for (int i = 0; i < items_valid; ++i)   out << items[i];
       return out << endl;
     }

   enum
      {
        ADD_ALLOC = 4,   ///< extra chars added when extending the string
        MIN_ALLOC = 16,   ///< min. size allocated
      };

   /// double the allocated size
   void extend()
      {
        items_allocated += items_allocated;
        T * new_items = new T[items_allocated];
        _copy(new_items, items, items_valid);
        delete [] items;
        items = new_items;
      }

   /// increase the allocated size to at least new_size
   void extend(unsigned int new_size)
      {
        if (new_size <= items_allocated)   return;

        items_allocated = new_size + ADD_ALLOC;
        T * new_items = new T[items_allocated];
        _copy(new_items, items, items_valid);
        delete [] items;
        items = new_items;
      }

   /// copy \b count characters
   static void _copy(T * dst, const T * src, int count)
      {
        Assert(count >= 0);
        for (int c = 0; c < count; ++c)  copy_1(dst[c], src[c], LOC);
        
      }

   /// copy \b count characters downwards (for oveerlapping src/dst
   static void copy_downwards(T * dst, const T * src, int count)
      {
        Assert(count >= 0);
        for (int c = count - 1; c >= 0; --c)  dst[c] = src[c];
      }

   /// the number of characters allocated
   int items_allocated;

   /// the number of characters valid (always <= items_allocated)
   int items_valid;

   /// the items
   T * items;
};
//-----------------------------------------------------------------------------

#endif // __SIMPLE_STRING_HH_DEFINED__

