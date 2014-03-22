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

#include "Id.hh"
#include "Value.hh"
#include "Workspace.hh"

/**
 Some objects must be constructed before others, because the latter rely
 on the former. This file contains all static objects with such requirements
 on the order of construction.

 See also 3.6.2 of "ISO standard Programming Languages — C++"
 **/

// 1. the anchors for values and index expressions are used by
//    static Value constructors.
DynamicObject DynamicObject::all_values(LOC);
DynamicObject DynamicObject::all_index_exprs(LOC);

// 2. static APL values must exist before system variables (in Workspace)
//    because some system variable constructors use them.
//
#define stv_def(x) Value Value:: _ ## x(LOC, Value::Value_how_ ## x); \
                   Value_P Value:: x ## _P(&Value::_## x, LOC);
#include "StaticValues.def"

// 3. Id strings are used by function id_name() which is used by system
//    variable constructors (in Workspace)
//
#define av(x, u) const UCS_string id_ ## x (UNI_ ## u);
#define pp(x, u) const UCS_string id_ ## x (UTF8_string(#x));
#define qf(x, u) const UCS_string id_QUAD_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define qv(x, u) const UCS_string id_QUAD_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define st(x, u) const UCS_string id_ ## x (UTF8_string(u));

#define id_def(_id, _uni, _val, _mac)   _mac(_id, _uni)
#include "Id.def"

//-----------------------------------------------------------------------------
const UCS_string &
id_name(Id id)
{
   switch(id)
      {
#define av(x) case ID_ ## x: return id_ ## x;
#define pp(x) case ID_ ## x: return id_ ## x;
#define qf(x) case ID_QUAD_ ## x: return id_QUAD_ ## x;
#define qv(x) case ID_QUAD_ ## x: return id_QUAD_ ## x;
#define st(x) case ID_ ## x: return id_ ## x;

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   CERR << "Unknown Id " << HEX(id);
   Assert(0 && "Bad Id");
}

// 4. now Workspace can be constructed
//
Workspace Workspace::the_workspace;


