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

#ifndef __ID_HH_DEFINED__
#define __ID_HH_DEFINED__

#include <iostream>

class Function;
class Symbol;
class UCS_string;

/*!
 An Identifier for each internal object (primitives, Quad-symbols, and more).
 The ID can be derived in four ways:

 1. from the name of an ⎕AV element, e.g.  ID_F2_AND or ID_ASSIGN
 2. from a name,                     e.g.  ID_APL_VALUE or ID_CHARACTER
 3. from a distinguished var name,   e.g.  ID_QUAD_AI or ID_QUAD_AV
 4. from a distinguished fun name,   e.g.  ID_QUAD_AT or ID_QUAD_EM
 5. from a special token name              ID_L_PARENT1 or ID_R_PARENT1

  This is controlled by 5 corresponding macros: av() pp() qv() qf() resp. st()
 */

enum Id
{
#define av(x, v) ID_      ## x v,
#define pp(x, v) ID_      ## x v,
#define qf(x, v) ID_QUAD_ ## x v,
#define qv(x, v) ID_QUAD_ ## x v,
#define st(x, v) ID_      ## x v,

#define id_def(id, _uni, val, mac) mac(id, val)
#include "Id.def"
};

/// return a pointer to the system function with Id \b id,
/// or 0 if id is no function
const UCS_string & id_name(Id id);

/// If \b id is the ID of primitive function, primitive operator, or
/// quad function, then return a pointer to it. If not, return 0.
Function * get_system_function(Id id);

/// If \b id is the ID of a quad variable, then return a pointer to its symbol
Symbol * get_system_variable(Id id);

#endif // __ID_HH_DEFINED__

