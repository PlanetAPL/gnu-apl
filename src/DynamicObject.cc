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


#include "Common.hh"
#include "DynamicObject.hh"
#include "PrintOperator.hh"
#include "Value.hh"   // so that casting to Value * works

//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const DynamicObject & dob)
{
   dob.print(out);
   return out;
}
//-----------------------------------------------------------------------------
void
DynamicObject::print_chain(ostream & out) const
{
int pos = 0;
   for (const DynamicObject * p = this; ;)
       {
         out << "    Chain[" << setw(2) << pos++ << "]  " 
             << (void *)p->prev << " --> "
             << (void *)p << " --> "   << (void *)p->next << "    "
             << p->where_allocated() << endl;

          p = p->next;
          if (p == this)   break;
       }
}
//-----------------------------------------------------------------------------
void
DynamicObject::print_new(ostream & out, const char * loc) const
{
   out << "new    " << (const void *)(const Value *)this
       << " at " << loc << endl;
}
//-----------------------------------------------------------------------------
void
DynamicObject::print(ostream & out) const
{
   out << "DynamicObject: " << (void *)this
       << " (Value: " << (void *)(Value *)this << ") :" << endl
       << "    prev:      " << (void *)prev         << endl
       << "    next:      " << (void *)next         << endl
       << "    allocated: " << where_allocated()    << endl;
}
//-----------------------------------------------------------------------------

