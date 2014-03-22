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


#include "Assert.hh"
#include "NamedObject.hh"
#include "Token.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
NameClass
NamedObject::get_nc() const
{
   if (id == ID_USER_SYMBOL)   // user defined object
      {
        const Symbol * sym = get_symbol();
        if (sym == 0)   return NC_UNUSED_USER_NAME;
        const ValueStackItem * tos = sym->top_of_stack();
        if (tos)   return tos->name_class;
        return NC_UNUSED_USER_NAME;
      }

   // Distinguished name.
   //
   Assert(Avec::is_quad(idname[0]));

int len;
Token tok = Workspace::get_quad(idname, len);
   if (len == 1)   return NC_INVALID;

   if (tok.get_Class() == TC_SYMBOL)   return NC_VARIABLE;
   if (tok.get_Class() == TC_FUN2)     return NC_FUNCTION;
   if (tok.get_Class() == TC_FUN1)     return NC_FUNCTION;
   if (tok.get_Class() == TC_FUN0)     return NC_FUNCTION;

   return NC_INVALID;
}
//-----------------------------------------------------------------------------
