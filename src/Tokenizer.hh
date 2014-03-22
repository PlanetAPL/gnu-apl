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

#ifndef __TOKENIZER_HH_DEFINED__
#define __TOKENIZER_HH_DEFINED__

#include "Token.hh"

/*!
     Functions related to tokenizing of input lines and user defined functions.
 */
class Tokenizer
{
public:
   /// Constructor
   Tokenizer(ParseMode pm, const char * _loc)
   : pmode(pm),
     loc(_loc),
     rest_1(0),
     rest_2(0)
   {}

   /// tokenize UTF-8 string \b input into token string \b tos.
   ErrorCode tokenize(const UCS_string & input, Token_string & tos);

protected:
   /// tokenize UTF-8 string \b input into token string \b tos.
   void do_tokenize(const UCS_string & input, Token_string & tos);

   /// tokenize a function
   void tokenize_function(Source<Unicode> & src, Token_string & tos);

   /// tokenize a QUAD function or variable
   void tokenize_quad(Source<Unicode> & src, Token_string & tos);

   /// tokenize a single quoted string
   void tokenize_string1(Source<Unicode> & src, Token_string & tos);

   /// tokenize a double quoted string
   void tokenize_string2(Source<Unicode> & src, Token_string & tos);

   /// tokenize a number (integer, floating point, or complex).
   void tokenize_number(Source<Unicode> & src, Token_string & tos);

   /// tokenize a real number (integer or floating point).
   bool tokenize_real(Source<Unicode> &src, APL_Float & flt_val,
                      APL_Integer & int_val);

   /// tokenize a symbol
   void tokenize_symbol(Source<Unicode> & src, Token_string & tos);

   /// the parsing mode of this parser
   const ParseMode pmode;

   const char * loc;
   int rest_1;
   int rest_2;
};

#endif // __TOKENIZER_HH_DEFINED__
