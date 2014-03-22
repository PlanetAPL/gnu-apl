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

#ifndef __PRINTOPERATOR_HH_DEFINED
#define __PRINTOPERATOR_HH_DEFINED

#include <iostream>

#include "Common.hh"
#include "Id.hh"
#include "Source.hh"
#include "TokenEnums.hh"
#include "Unicode.hh"

class Cell;
class DynamicObject;
struct Format_sub;
class Function;
struct Function_PC2;
class IndexExpr;
class LineLabel;
class PrintBuffer;
class Shape;
template <class T> class Source;
class Symbol;
class Token;
class UCS_string;
class UTF8_string;
class Value;

ostream & operator << (ostream &, const Format_sub &);
ostream & operator << (ostream &, const Function &);
ostream & operator << (ostream &, const Function_PC2 &);
ostream & operator << (ostream &, const Cell &);
ostream & operator << (ostream &, const DynamicObject &);
ostream & operator << (ostream &,       Id id);
ostream & operator << (ostream &, const IndexExpr &);
ostream & operator << (ostream &, const LineLabel &);
ostream & operator << (ostream &, const PrintBuffer &);
ostream & operator << (ostream &, const Shape &);
ostream & operator << (ostream &, const Symbol &);
ostream & operator << (ostream &, const Token &);
ostream & operator << (ostream &,       TokenTag);
ostream & operator << (ostream &,       TokenClass);
ostream & operator << (ostream &, const UCS_string &);
ostream & operator << (ostream &,       Unicode);
ostream & operator << (ostream &, const UTF8_string &);
ostream & operator << (ostream &, const Value & value);

inline ostream & operator << (ostream & out,       ValueFlags flags)
   { return print_flags(out, flags); }

template<typename T>
ostream & operator << (ostream & out, const Source<T> & src)
   { loop(s, src.rest())   out << src[s];   return out; }

#endif // __PRINTOPERATOR_HH_DEFINED
