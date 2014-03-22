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

#ifndef __PRINT_CONTEXT_HH_DEFINED__
#define __PRINT_CONTEXT_HH_DEFINED__

#include "Common.hh"
#include "SystemLimits.hh"

class Workspace;

/// PrintContext is a collection of attributes that control how
/// APL values are being printed.
class PrintContext
{
public:
   /// default constructor from ⎕PS, ⎕PP, ⎕CT, and ⎕PW of workspace \b ws.
   /// implementation is in Workspace.cc !!
   PrintContext();

   /// constructor from another style
   PrintContext(PrintStyle st)
   : style(st),
     precision(DEFAULT_QUAD_PP),
     width(DEFAULT_QUAD_PW)
   {}

   /// constructor for given values
   PrintContext(PrintStyle st, int qpp, double pct, int w)
   : style(st),
     precision(qpp),
     width(w)
   {}

   /// return the print style to be used
   PrintStyle get_style() const { return style; }

   /// set the print style to be used
   void set_style(PrintStyle ps) { style = ps; }

   /// return true iff exponential format shall be used
   bool get_scaled() const   { return (style & PST_SCALED) != 0; }

   /// request scaled (exponential) format
   void set_scaled()   { style = PrintStyle(style | PST_SCALED); }

   /// return the print precision to be used
   int get_PP() const { return precision; }

   /// return the print width to be used
   int get_PW() const { return width; }

protected:
   /// the print style to be used
   PrintStyle style;

   /// the print precision to be used
   int precision;   // aka. ⎕PP

   /// the print width to be used
   int width;          // aka. ⎕PW
};

#endif // __PRINT_CONTEXT_HH_DEFINED__
