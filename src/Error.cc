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

#include "Common.hh"
#include "Error.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
void
Error::print(ostream & out) const
{
   Log(LOG_verbose_error)
      {
        out << endl
            << "--------------------------" << endl;

        if (error_code == E_NO_ERROR)
           {
             out << error_name(E_NO_ERROR) << endl
                 << "--------------------------" << endl;

             return;
           }

        if (error_message_1.size())   out << error_message_1 << endl;
        else                          out << error_name(error_code) << endl;

        if (parser_loc)   out << "   Parser LOC: " << parser_loc << endl;
        if (print_loc)    out << "   Print LOC:  " << print_loc  << endl;

        if (symbol_name.size())
                          out << "   Symbol:     " << symbol_name << endl;

                          out << "   Thrown at:  " << throw_loc << endl
                              << "--------------------------"   << endl
                              << endl;
      }
}
//-----------------------------------------------------------------------------
const UCS_string
Error::error_name(ErrorCode err)
{
   switch(err)
      {
   /// the cases
#define err_def(c, txt, _maj, _min) \
   case E_ ## c:   return UCS_string(UTF8_string(_(txt)));

#include "Error.def"
      }

   return UCS_string(_("Unknown Error"));
}
//-----------------------------------------------------------------------------
bool
Error::is_known() const
{
   switch(error_code)
      {
   /// the cases
#define err_def(c, _txt, _maj, _min) case  E_ ## c:   return true;
#include "Error.def"
      }

   return false;
}
//-----------------------------------------------------------------------------
bool
Error::is_syntax_or_value_error() const
{
   if (error_major(error_code) == 2)   return true;   // some SYNTAX ERROR
   if (error_major(error_code) == 3)   return true;   // VALUE ERROR
   return false;
}
//-----------------------------------------------------------------------------
void
Error::init(ErrorCode ec, const char * loc)
{
   error_code = ec;
   throw_loc = loc;
   error_message_1 = error_name(error_code);
   error_message_2.clear();
   symbol_name.clear();
   parser_loc = 0;
   show_locked = false;
   left_caret = -1;
   right_caret = -1;
   print_loc = 0;
}
//-----------------------------------------------------------------------------
UCS_string
Error::get_error_line_3() const
{
   if (error_code == E_NO_ERROR)   return UCS_string();

   if (left_caret < 0)   return UCS_string();

UCS_string ret;
   if (left_caret > 0)   ret.append(UCS_string(left_caret, UNI_ASCII_SPACE));
   ret.append(UNI_ASCII_CIRCUMFLEX);

const int diff = right_caret - left_caret;
   if (diff <= 0)   return ret;
   if (diff > 1)   ret.append(UCS_string(diff - 1, UNI_ASCII_SPACE));
   ret.append(UNI_ASCII_CIRCUMFLEX);

   return ret;
}
//-----------------------------------------------------------------------------
void
Error::print_em(ostream & out, const char * loc)
{
   if (print_loc) 
      {
        CERR << endl << "*** Error printed twice; first printed at "
             << print_loc << endl;

        return;
      }

   print_loc = loc;
   if (get_error_line_1().size())   out << get_error_line_1() << endl;

   out << get_error_line_2() << endl
       << get_error_line_3() << endl;
}
//-----------------------------------------------------------------------------
void
throw_apl_error(ErrorCode code, const char * loc) throw(Error)
{
   ADD_EVENT(0, VHE_Error, code, loc);

StateIndicator * si = Workspace::SI_top();

   Log(LOG_error_throw)
      {
        CERR << endl
             << "throwing " << Error::error_name(code)
             << " at " << loc << endl;
      }

   Log(LOG_verbose_error)
      {
        if (!(si && si->get_safe_execution()))
           Backtrace::show(__FILE__, __LINE__);
      }

   // maybe map error to DOMAIN ERROR.
   //
   if (si)
      {
        const UserFunction * ufun = si->get_executable()->get_ufun();
        if (ufun && ufun->get_exec_properties()[3])   code = E_DOMAIN_ERROR;
      }

Error error(code, loc);
   if (si)   si->update_error_info(error);
   throw error;
}
//-----------------------------------------------------------------------------
void
throw_parse_error(ErrorCode code, const char * par_loc, const char *loc) throw(Error)
{
   Log(LOG_error_throw)
      CERR << endl
           << "throwing " << Error::error_name(code) << " at " << loc << endl;

   Log(LOG_verbose_error)   Backtrace::show(__FILE__, __LINE__);

Error error(code, loc);
   error.parser_loc = par_loc;

// StateIndicator * si = Workspace::SI_top();
//   if (si)   si->update_error_info(error);
   throw error;
}
//-----------------------------------------------------------------------------
void
throw_symbol_error(const UCS_string & sym_name, const char * loc) throw(Error)
{
   Log(LOG_error_throw)   
      {   
        CERR << "throwing VALUE ERROR at " << loc;
        if (sym_name.size())   CERR << " (symbol is " << sym_name << ")"; 
        CERR << endl;

      }

   Log(LOG_verbose_error)     Backtrace::show(__FILE__, __LINE__);

Error error(E_VALUE_ERROR, loc);
   error.symbol_name = sym_name;
   if (Workspace::SI_top())   Workspace::SI_top()->update_error_info(error);
   throw error;
}
//-----------------------------------------------------------------------------
void
throw_define_error(const UCS_string & fun_name, const UCS_string & cmd,
                   const char * loc) throw(Error)
{
   Log(LOG_error_throw)   
      {   
        CERR << "throwing DEFN ERROR at " << loc
             << " (function is " << fun_name << ")" << endl;
      }

   Log(LOG_verbose_error)     Backtrace::show(__FILE__, __LINE__);

Error error(E_DEFN_ERROR, loc);
   error.symbol_name = fun_name;
   error.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
   error.error_message_2.append(cmd);   // something like ∇FUN[⎕]∇
   error.left_caret = error.error_message_2.size() - 1;
   if (Workspace::SI_top())   *Workspace::get_error() = error;
   Workspace::update_EM_ET(error);   // update ⎕EM and ⎕ET
   throw error;
}
//-----------------------------------------------------------------------------

