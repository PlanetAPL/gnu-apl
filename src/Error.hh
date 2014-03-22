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

#ifndef __ERROR_HH_DEFINED__
#define __ERROR_HH_DEFINED__

#include "Backtrace.hh"
#include "Common.hh"
#include "ErrorCode.hh"
#include "UCS_string.hh"

class Function;
class IndexExpr;

/**
 ** The exception that is thrown when errors occur. 
 ** The primary item is the error_code; the other items are only valid if
 ** error_code != NO_ERROR
 **/
struct Error
{
   /// constructor: error with error code ec
   Error(ErrorCode ec, const char * loc) { init(ec, loc); }

   /// initialize this error: set mandatory items and clear optional items
   void init(ErrorCode ec, const char * loc);

   /// return true iff error_code is known (as opposed to an arbitrary
   /// error code constructed by ⎕ES)
   bool is_known() const;

   /// return true iff error_code is some SYNTAX ERROR or VALUE ERROR
   bool is_syntax_or_value_error() const;

   /// set error line 2 to ucs
   void set_error_line_2(const UCS_string & ucs, int lcaret, int rcaret)
      { error_message_2 = ucs; left_caret = lcaret; right_caret = rcaret; }

   /// return ⎕EM[1;]. This is the first of 3 error lines. It contains the
   /// error name (like SYNTAX ERROR, DOMAIN ERROR, etc) and is subject
   /// to translation
   const UCS_string & get_error_line_1() const
      { return error_message_1; }

   /// return error_message_2. This is the second of 3 error lines.
   /// It contains the failed statement and is NOT subject to translation.
   const UCS_string & get_error_line_2() const
      { return error_message_2; }

   /// compute the caret line. This is the thirs of 3 error lines.
   /// It contains the failure position the statement and is NOT subject
   /// to translation.
   UCS_string get_error_line_3() const;

   /// print the error and its related information
   void print(ostream & out) const;

   /// return a string describing the error
   static const UCS_string error_name(ErrorCode err);

   /// return the major class (⎕ET) of the error
   static int error_major(ErrorCode err)
      { return err >> 16; }

   /// return the category (⎕ET) of the error
   static int error_minor(ErrorCode err)
      { return err & 0x00FF; }

   /// print the 3 error message lines as per ⎕EM
   void print_em(ostream & out, const char * loc);

   /// the error code
   ErrorCode error_code;

   /// the mandatory source file location where the error was thrown
   const char * throw_loc;

   /// an optional symbol related to (i.e. triggering) the error
   UCS_string symbol_name;

   /// an optional source file location (for parse errors)
   const char * parser_loc;

   /// an optional error text. this text is provided for non-APL errors,
   // for example in ⎕ES
   UCS_string error_message_1;

   /// second line of error message (only valid if error_code != _NO_ERROR)
   UCS_string error_message_2;

   /// display user function and line rather than ⎕ES instruction
   bool show_locked;

   /// the left caret position (-1 if none) for the error display
   int left_caret;

   /// the right caret position (-1 if none) for the error display
   int right_caret;

   const char * get_print_loc() const
      { return print_loc; }

protected:
   const char * print_loc;

private:
   /// constructor (not implemented): prevent construction without error code
   Error();
};

/// throw an Error with \b code only (typically a standard APL error)
void throw_apl_error(ErrorCode code, const char * loc) throw(Error)
#ifdef __GNUC__
    __attribute__ ((noreturn))
#endif
;

/// throw a error with a parser location
void throw_parse_error(ErrorCode code, const char * par_loc,
                       const char * loc) throw(Error)
#ifdef __GNUC__
    __attribute__ ((noreturn))
#endif
;

/// throw an Error related to \b Symbol \b symbol
void throw_symbol_error(const UCS_string & symb, const char * loc) throw(Error)
#ifdef __GNUC__
    __attribute__ ((noreturn))
#endif
;

/// throw a define error for function \b fun
void throw_define_error(const UCS_string & fun, const UCS_string & cmd,
                        const char * loc) throw(Error)
#ifdef __GNUC__
    __attribute__ ((noreturn))
#endif
;

#define ATTENTION           throw_apl_error(E_ATTENTION,            LOC)
#define AXIS_ERROR          throw_apl_error(E_AXIS_ERROR,           LOC)
#define DEFN_ERROR          throw_apl_error(E_DEFN_ERROR,           LOC)
#define DOMAIN_ERROR        throw_apl_error(E_DOMAIN_ERROR,         LOC)
#define INDEX_ERROR         throw_apl_error(E_INDEX_ERROR,          LOC)
#define INTERRUPT           throw_apl_error(E_INTERRUPT,            LOC)
#define LENGTH_ERROR        throw_apl_error(E_LENGTH_ERROR,         LOC)
#define LIMIT_ERROR_RANK    throw_apl_error(E_SYSTEM_LIMIT_RANK,    LOC)
#define LIMIT_ERROR_FUNOPER throw_apl_error(E_SYSTEM_LIMIT_FUNOPER, LOC)
#define RANK_ERROR          throw_apl_error(E_RANK_ERROR,           LOC)
#define SYNTAX_ERROR        throw_apl_error(E_SYNTAX_ERROR,         LOC)
#define LEFT_SYNTAX_ERROR   throw_apl_error(E_LEFT_SYNTAX_ERROR,    LOC)
#define SYSTEM_ERROR        throw_apl_error(E_SYSTEM_ERROR,         LOC)
#define TODO                throw_apl_error(E_NOT_YET_IMPLEMENTED,  LOC)
#define FIXME               {  Backtrace::show(__FILE__, __LINE__); \
                               exit(0); \
                               throw_apl_error(E_THIS_IS_A_BUG,        LOC); }
#define VALUE_ERROR         throw_apl_error(E_VALUE_ERROR,          LOC)
#define VALENCE_ERROR       throw_apl_error(E_VALENCE_ERROR,        LOC)

#endif // __ERROR_HH_DEFINED__
