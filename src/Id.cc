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

#include "Avec.hh"
#include "Common.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_SVx.hh"
#include "Quad_TF.hh"
#include "SkalarFunction.hh"
#include "UCS_string.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, Id id)
{
   return out << id_name(id);
}
//-----------------------------------------------------------------------------
Function *
get_system_function(Id id)
{
   switch(id)
      {
#define av(x) case ID_ ## x: return &Bif_F12_ROLL::fun;
#define pp(x)
#define qf(x) case ID_QUAD_ ## x: return &Quad_ ## x::fun;
#define qv(x)
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
get_system_variable(Id id)
{
   switch(id)
      {
#define av(x)
#define pp(x)
#define qf(x)
#define qv(x) case ID_QUAD_ ## x:return &Workspace::get_v_quad_ ## x();
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   return 0;
}
//-----------------------------------------------------------------------------
