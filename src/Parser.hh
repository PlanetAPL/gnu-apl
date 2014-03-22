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

#ifndef __PARSER_HH_DEFINED__
#define __PARSER_HH_DEFINED__

#include "Token.hh"

/*!
     Functions related to parsing of input lines and user defined functions.
 */
class Parser
{
public:
   /// Constructor
   Parser(ParseMode pm, const char * loc)
   : pmode(pm),
     create_loc(loc)
   {}

   /// Parse UTF-8 string \b input into token string \b tos.
   ErrorCode parse(const UCS_string & input, Token_string & tos);

   /// print the array \b token of \b Token into \b out.
   static void print_token_list(ostream & out, const Token_string & token,
                         uint32_t from = 0);

protected:
   /// Parse token string \b tos (a statement without diamonds).
   ErrorCode parse_statement(Token_string & tos);

   /// find opening bracket; throw error if not found
   static int32_t find_opening_bracket(Token_string & string, int32_t pos);

   /// find closing bracket; throw error if not found
   static int32_t find_closing_bracket(Token_string & string, int32_t pos);

   /// find opening paranthesis; throw error if not found.
   static int32_t find_opening_parent(Token_string & string, int32_t pos);

   /// find closing paranthesis; throw error if not found.
   static int32_t find_closing_parent(Token_string & string, int32_t pos);

   /// Collect consecutive smaller APL values or value token into vectors
   void collect_constants(Token_string & tok);

   /// mark next symbol left of ← on the same level as LSYMB
   static void mark_lsymb(Token_string & tok);

   /// Collect groups
   bool collect_groups(Token_string & tok);

   /// Replace (X) by X and ((..) by (..) in \b string. (X is a single token)
   static void remove_nongrouping_parantheses(Token_string & string);

   /// remove VOID token from \b token (compacting \b token)
   static void remove_void_token(Token_string & token);

   /// Create one APL value from \b len values.
   void create_value(Token_string & token, uint32_t pos, uint32_t len);

   /// Create one skalar APL value from a token.
   void create_skalar_value(Token & tok);

   /// Create one vector APL value from \b value_count single values.
   void create_vector_value(Token_string & token, uint32_t pos, uint32_t len);

   /// the parsing mode of this parser
   const ParseMode pmode;

   /// where this parser was constructed
   const char * create_loc;
};

#endif // __PARSER_HH_DEFINED__
