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

// this file #includes those header files that are of interest for
// native functions

   // helpers
   //
#include "Common.hh"

   // Cells are the elements of  the ravel of an APL value
   //
#include "Cell.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "PointerCell.hh"
#include "RealCell.hh"

   // errors that may be returned or thrown....
   //
#include "Error.hh"

   // Functions are primitive or user defined functions and operators
   //
#include "Function.hh"

   // Token are the result of functions and the parsed items in the body
   // of user defined functions  and operators
   //
#include "Token.hh"

   // Values are the APL values (nested or not) manipulated by Functions.
#include "Value.hh"

   // access to system variables (⎕IO and friends)
#include "Workspace.hh"

