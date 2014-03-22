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

#ifndef __SHARED_VALUE_POINTER_HH_DEFINED__
#define __SHARED_VALUE_POINTER_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
#error this file must only be included by Common.hh
#endif

class Value;

#define ptr_clear(p, l) p.reset()

int  get_owner_count(const Value * v);
void increment_owner_count(Value * v, const char * loc);
void decrement_owner_count(Value * v, const char * loc);

class Value_P
{
public:
   /// Constructor: 0 pointer
   Value_P()
   : value_p(0)
   {}

   /// Constructor: from Value *
   Value_P(Value * val)
     : value_p(val)
     {
       if (value_p)
          {
            increment_owner_count(value_p, LOC);
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), LOC);
          }
     }

   /// Constructor: from Value *
   Value_P(Value * val, const char * loc)
     : value_p(val)
     {
       if (value_p)
          {
            increment_owner_count(value_p, loc);
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), loc);
          }
     }

   /// Constructor: from other Value_P
   Value_P(const Value_P & other)
   : value_p(other.value_p)
   {
     if (value_p)
        {
          increment_owner_count(value_p, LOC);
          ADD_EVENT(value_p, VHE_PtrCopy1, use_count(), LOC);
        }
   }

   /// copy operator
   Value_P & operator =(const Value_P & other)
   {
      if (value_p == other.value_p)   return *this;   // same pointer

      if (value_p)   // override existing pointer
         {
           decrement_owner_count(value_p, LOC);
           ADD_EVENT(value_p, VHE_PtrClr, other.use_count(), LOC);
         }
          
      value_p = other.value_p;
      if (value_p)
         {
           increment_owner_count(value_p, LOC);
           ADD_EVENT(value_p, VHE_PtrCopy3, use_count(), LOC);
         }

      return *this;
   }

   int use_count() const
      { return get_owner_count(value_p); }

   /// Destructor
   ~Value_P()
      {
        if (value_p)
           {
             decrement_owner_count(value_p, LOC);
             ADD_EVENT(value_p, VHE_PtrDel, use_count(), LOC);
          }
      }

   void reset()
      {
        if (value_p)
           {
             decrement_owner_count(value_p, LOC);
             value_p = 0;
           }
      }

   const Value * operator->()  const
      { return value_p; }

   Value * operator->()
      { return value_p; }

   const Value & operator*()  const
      { return *value_p; }

   const Value * get() const
      { return value_p; }

   Value * get()
      { return value_p; }

 bool operator!() const
      { return value_p == 0; }


   bool operator ==(const Value_P & other) const
      { return value_p == other.value_p; }

   bool operator !=(const Value_P & other) const
      { return value_p != other.value_p; }

   void clear(const char * loc)
     {
       if (value_p)
          {
            decrement_owner_count(value_p, LOC);
            ADD_EVENT(value_p, VHE_PtrClr, use_count(), loc);

            value_p = 0;
          }
    }

protected:
   Value * value_p;
};

/// macro to facilitate Value_P in unions
#define VALUE_P(x) char u_ ## x[sizeof(Value_P)]; \
   Value_P & _ ## x() const { return *(Value_P *) & u_ ## x; }

//-----------------------------------------------------------------------------

#endif // __SHARED_VALUE_POINTER_HH_DEFINED__
