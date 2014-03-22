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

#ifndef __NAMED_OBJECT_HH_DEFINED__
#define __NAMED_OBJECT_HH_DEFINED__

#include "Id.hh"
#include "Value.hh"

class Function;
class Symbol;
class UCS_string;
class Value;

//-----------------------------------------------------------------------------
/**
     The possible values returned by \b QUAD-NC.
 */
enum NameClass
{
  NC_INVALID          = -1,   ///< invalid name class.
  NC_UNUSED_USER_NAME =  0,   ///< unused user name eg. pushed but not assigned
  NC_LABEL            =  1,   ///< Label.
  NC_VARIABLE         =  2,   ///< (assigned) variable.
  NC_FUNCTION         =  3,   ///< (user defined) function.
  NC_OPERATOR         =  4,   ///< (user defined) operator.
  NC_SHARED_VAR       =  5,   ///< shared variable.
};
//-----------------------------------------------------------------------------
/**
 A named object is something with a name.
 The name can be user defined or system defined.

  User define names are handled by class Symbol, while
   system defined names are handled by class Id.

  User define names are used for (user-defined) variables, functions,
  or operators.

   System names are used by system variables and dunctions (⎕xx) and
   by primitive functions and operators.
 **/

class NamedObject
{
public:
   /// constructor from Id
   NamedObject(Id i)
   : id(i),
     idname(id_name(i))
   {}

   /// return the name of the named object
   virtual const UCS_string & get_name() const
      { return id_name(id); }

   /// return the function for this Id (if any) or 0 if this Id does
   /// (currently) represent a function.
   virtual const Function * get_function() const  { return 0; }

   /// return the variable value for this Id (if any) or 0 if this Id does
   /// not (currently) represent a variable.
   virtual Value_P get_value()     { return Value_P(); }

   /// return the symbol for this user defined symbol (if any) or 0 if this Id
   /// refers to a system name
   virtual Symbol * get_symbol()   { return 0; }

   /// return the symbol for this user defined symbol (if any) or 0 if this Id
   /// refers to a system name
   virtual const Symbol * get_symbol() const  { return 0; }

   /// return the Id of this object (ID_USER_SYMBOL for user defined objects)
   Id get_Id() const
      { return id; }

   /// return true, iff this object is user-defined
   bool is_user_defined() const
      { return id == ID_USER_SYMBOL; }

   /// Get current \b NameClass of \b this name.
   NameClass get_nc() const;

   /// the object's id
   const Id id;

   /// the name of the id
   const UCS_string & idname;
};

#endif // __NAMED_OBJECT_HH_DEFINED__
