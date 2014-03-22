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

#ifndef __DYNAMIC_OBJECT_HH_DEFINED__
#define __DYNAMIC_OBJECT_HH_DEFINED__

#include "Common.hh"
#include "Output.hh"
#include "PrintOperator.hh"

/*
 A base class for dynamically allocated objects. It remembers where an object
 was allocated and maintains a doubly linked ring of all allocated objects.

 The doubly linked list has a statically allocated anchor that must not
 be removed. I.e. the list is never empty.
 */
/// A Value or an IndexExpr
class DynamicObject
{
public:
   /// constructor: a DynamicObject allocated at source location \b loc
   DynamicObject(const char * loc, DynamicObject * anchor)
   : alloc_loc(loc)
   {
     Log(LOG_delete)   print_new(CERR, loc);

     next = anchor->next;
     anchor->next = this;

     prev = anchor;
     next->prev = this;
   }

   /// a special constructor for statically allocated objects.
   DynamicObject(const char * loc)
   : alloc_loc(loc),
     next(this),
     prev(this)
   {}

   /// return the next DynamicObject in its list
   DynamicObject * get_next()              { return next; }

   /// return the next DynamicObject in its list
   const DynamicObject * get_next()  const { return next; }

   /// return the previous DynamicObject in its list
   DynamicObject * get_prev()             { return prev; }

   /// return the previous DynamicObject in its list
   const DynamicObject * get_prev() const { return prev; }

   /// unlink this DynamicObject from its list
   void unlink()
      {
        // print(CERR);

        prev->next = next;
        next->prev = prev;

        prev = this;
        next = this;
      }

   /// print this object
   ///
   void print(ostream & out) const;

   /// print new object message
   ///
   void print_new(ostream & out, const char * loc) const;

   /// print the list of objects starting at this object
   void print_chain(ostream & out) const;

   /// where this value was allocated.
   const char * where_allocated() const
      { return alloc_loc; }

   /// return a pointer to all allocated Values
   static const DynamicObject * get_all_values()
      { return &all_values; }

protected:
   /// where this value was allocated or deleted.
   const char * alloc_loc;

   /// the next DynamicObject (of all objects) in \b this's list
   DynamicObject * next;

   /// the previous DynamicObject (of all objects) in \b this's list
   DynamicObject * prev;

   /// the anchor for all dynamic Value instances
   static DynamicObject all_values;

   /// the anchor for all dynamic IndexExpr instances
   static DynamicObject all_index_exprs;
};

#endif // __DYNAMIC_OBJECT_HH_DEFINED__
