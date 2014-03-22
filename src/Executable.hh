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

#ifndef __EXECUTABLE_HH_DEFINED__
#define __EXECUTABLE_HH_DEFINED__

#include "Output.hh"
#include "Parser.hh"

class UCS_string;
class UserFunction;

//-----------------------------------------------------------------------------
/**
     A sequence of APL token. An executable is created for one of 3 purposes:
     - an APL expression for execute (⍎), or
     - the statements of one line in immediate execution, or
     - all lines of a user defined function.
 **/
class Executable
{
public:
   /// constructor
   Executable(const UCS_string & ucs, const char * loc);

   /// destructor: release values held by the body
   virtual ~Executable();

   /// the parse mode for \b this Executable
   virtual ParseMode get_parse_mode() const = 0;

   /// return the name of this function, or ◊ or ⍎
   virtual const UCS_string & get_name() const = 0;

   /// return the result symbol iff this is a user defined function or
   /// operator that returns a value
   virtual Symbol * get_sym_Z() const
      { return 0; }

   /// return the function name symbol iff this is a user defined function
   virtual Symbol * get_sym_FUN() const
      { return 0; }

   /// return a UserFunction * (if \b this is one) or else 0.
   virtual const UserFunction * get_ufun() const
   { return 0; }

   /// return a UserFunction * (if \b this is one) or else 0.
   virtual UserFunction * get_ufun()
   { return 0; }

   /// get the line number for pc
   virtual Function_Line get_line(Function_PC pc) const
      { return Function_Line_0; }

   /// print this user defined executable to \b out
   virtual ostream & print(ostream & out) const;

   /// execute the body of this executable
   Token execute_body() const;

   /// return the body of this executable
   const Token_string & get_body() const
      { return body; }

   /// return number of function lines (including header line 0)
   const int get_text_size() const 
      { return text.size(); }

   /// return function line n (0 = header line)
   const UCS_string & get_text(int l) const
      { return text[l]; }

   /// return the text of statement at \b pc
   UCS_string statement_text(Function_PC pc) const;

   /// return the first PC of the \b line
   virtual Function_PC line_start(Function_Line line) const
      { return Function_PC_0; }

   /// compute lines 2 and 3 in \b error
   void set_error_info(Error & error, Function_PC2 range) const;

   /// return the start of the statement to which pc belongs
   Function_PC get_statement_start(int pc) const;

   /// clear marked flag in all body token
   void unmark_all_values() const;

   /// print all owners of \b value
   int show_owners(const char * prefix, ostream & out, const Value & value) const;

   /// say where this SI entry was allocated
   const char * get_loc() const
      { return alloc_loc; }

protected:
   /// where this SI entry was allocated
   const char * alloc_loc;

   /// parse the body line number \b line of \b this function
   void parse_body_line(Function_Line line, const UCS_string & ucs,
                        const char * loc);

   /// the program text from which \b body was created
   vector<UCS_string> text;

   /// The token to be executed. They are organized line by line and
   /// statement by statement, but the token within a statement reversed
   /// due to the right-to-left execution of APL.
   Token_string body;
};
//-----------------------------------------------------------------------------
/**
   The token of an execute expression (⍎',,,')
 **/
class ExecuteList : public Executable
{
   friend class XML_Loading_Archive;

public:
   /// compute body token from text \b data
   static ExecuteList * fix(const UCS_string & data, const char * loc);

   /// overloaded Executable::get_parse_mode()
   virtual ParseMode get_parse_mode() const
      { return PM_EXECUTE; }

protected:
   /// constructor
   ExecuteList(const UCS_string & txt, const char * loc)
   : Executable(txt, loc)
   {}

   /// overloaded Executable::get_name()
   virtual const UCS_string & get_name() const
      { return id_name(ID_F1_EXECUTE); }
};
//-----------------------------------------------------------------------------
/**
   The token of an statement list (cmd ◊ cmd ... ◊ cmd)
 **/
class StatementList : public Executable
{
   friend class XML_Loading_Archive;

public:
   /// compute body token from text \b data
   static StatementList * fix(const UCS_string & data, const char * loc);

   /// overloaded Executable::get_parse_mode()
   virtual ParseMode get_parse_mode() const
      { return PM_STATEMENT_LIST; }

protected:
   /// constructor
   StatementList(const UCS_string txt, const char * loc)
   : Executable(txt, loc)
   {}

   /// overloaded Executable::get_name()
   virtual const UCS_string & get_name() const
      { return id_name(ID_DIAMOND); }
};
//-----------------------------------------------------------------------------

#endif // __EXECUTABLE_HH_DEFINED__
