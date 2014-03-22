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

#include <stdio.h>
#include <vector>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "Error.hh"
#include "Output.hh"
#include "Parser.hh"
#include "StateIndicator.hh"
#include "Symbol.hh"
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
UserFunction::UserFunction(const UCS_string txt, int & error_line,
                           bool keep_existing, const char * loc)
  : Executable(txt, loc),
    Function(ID_USER_SYMBOL, TOK_FUN2),
    sym_Z(0),
    sym_A(0),
    sym_LO(0),
    sym_FUN(0),
    sym_RO(0),
    sym_X(0),
    sym_B(0)
{
   set_creation_time(now());

   exec_properties[0] = 0;
   exec_properties[1] = 0;
   exec_properties[2] = 0;
   exec_properties[3] = 0;

   line_starts.push_back(Function_PC_0);   // will be set later.

   error_line = 0;
   parse_header_line(get_text(0));

   // check that the function can be defined.
   //
   if (!sym_FUN->can_be_defined())   DEFN_ERROR;
     
Function * old_function = sym_FUN->get_function();
   if (old_function && keep_existing)    DEFN_ERROR;

   for (int l = 1; l < get_text_size(); ++l)
      {
        error_line = l;
        line_starts.push_back(Function_PC(body.size()));

        const UCS_string & text = get_text(l);
        parse_body_line(Function_Line(l), text, loc);
      }

   Log(LOG_UserFunction__fix)
      {
        CERR << "body.size() is " << body.size() << endl
             << "line_starts.size() is " << line_starts.size() <<endl; 
      }

   // let [0] be the end of the function.
   line_starts[0] = Function_PC(body.size());

   if (sym_Z)     body.append(Token(TOK_RETURN_SYMBOL, sym_Z), LOC);
   else           body.append(Token(TOK_RETURN_VOID), LOC);

   check_duplicate_symbols();

   if (sym_LO)   sym_FUN->set_nc(NC_OPERATOR, this);
   else          sym_FUN->set_nc(NC_FUNCTION, this);

   if (old_function)
      {
        const UserFunction * old_ufun = old_function->get_ufun1();
        Assert(old_ufun);
        delete old_ufun;
      }

   error_line = -1;   // assume no error
}
//-----------------------------------------------------------------------------
UserFunction::~UserFunction()
{
   Log(LOG_UserFunction__enter_leave)
      CERR << "Function " << get_name() << " deleted." << endl;
}
//-----------------------------------------------------------------------------
bool
UserFunction::is_operator() const
{
   return sym_LO != 0;
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_()
{
   Log(LOG_UserFunction__enter_leave)
      CERR << "Function " << get_name() << " calls eval_()" << endl;

   if (sym_B)   SYNTAX_ERROR;   // not defined niladic

   Workspace::push_SI(this, LOC);

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_B(Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_B("
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_LO)    SYNTAX_ERROR;   // defined as operator

   if (sym_B)   sym_B->push_value(B);
   if (sym_X)   sym_X->push();
   if (sym_A)   sym_A->push();

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_XB(Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_B("
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_LO)    SYNTAX_ERROR;   // defined as operator

   if (sym_B)   sym_B->push_value(B);
   if (sym_X)   sym_X->push_value(X);
   if (sym_A)   sym_A->push();

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_AB(Value_P A, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_AB("
             << Token(TOK_APL_VALUE1, A) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_LO)    SYNTAX_ERROR;   // defined as operator

   if (sym_B)   sym_B->push_value(B);
   if (sym_X)   sym_X->push();
   if (sym_A)   sym_A->push_value(A);

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_AB("
             << Token(TOK_APL_VALUE1, A) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_LO)    SYNTAX_ERROR;   // defined as operator

   if (sym_B)   sym_B->push_value(B);
   if (sym_X)   sym_X->push_value(X);
   if (sym_A)   sym_A->push_value(A);

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LB(Token & LO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_RO)    SYNTAX_ERROR;   // dyadic operator called monadically

                sym_B->push_value(B);
   if (sym_X)   sym_X->push();
   if (sym_A)   sym_A->push();
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (sym_RO)    SYNTAX_ERROR;   // dyadic operator called monadically

                sym_B->push_value(B);
   if (sym_X)   sym_X->push_value(X);
   if (sym_A)   sym_A->push();
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_A)    SYNTAX_ERROR;   // monadic function called dyadically
   if (sym_X)   sym_X->push();
   if (sym_RO)    SYNTAX_ERROR;   // defined as dyadic operator

   sym_B->push_value(B);
   sym_A->push_value(A);
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_A)    SYNTAX_ERROR;   // monadic function called dyadically
   if (sym_RO)    SYNTAX_ERROR;   // defined as dyadic operator

   sym_B->push_value(B);
   if (sym_X)   sym_X->push();
   sym_A->push_value(A);
   if (sym_X)   sym_X->push_value(X);
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_RO)    SYNTAX_ERROR;   // not defined as dyadic operator

                 sym_B->push_value(B);
   if (sym_X)    sym_X->push();
   if (sym_A)    sym_A->push();

   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());
   if (RO.is_function())   sym_RO->push_function(RO.get_function());
   else                    sym_RO->push_value(RO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_RO)    SYNTAX_ERROR;   // not defined as dyadic operator

                sym_B->push_value(B);
   if (sym_X)   sym_X->push_value(X);
   if (sym_A)   sym_A->push();
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());
   if (RO.is_function())   sym_RO->push_function(RO.get_function());
   else                    sym_RO->push_value(RO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_A)    SYNTAX_ERROR;    // defined monadic called dyadic
   if (sym_X)      sym_X->push();
   if (!sym_RO)    SYNTAX_ERROR;   // defined monadic op called dyadic

                 sym_B->push_value(B);
                 sym_A->push_value(A);
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());
   if (RO.is_function())   sym_RO->push_function(RO.get_function());
   else                    sym_RO->push_value(RO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALRXB(Value_P A, Token & LO, Token & RO,
                         Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!sym_A)    SYNTAX_ERROR;    // defined monadic called dyadic
   if (!sym_RO)    SYNTAX_ERROR;   // defined monadic op called dyadic

                 sym_B->push_value(B);
   if (sym_X)    sym_X->push_value(X);
                 sym_A->push_value(A);
   if (LO.is_function())   sym_LO->push_function(LO.get_function());
   else                    sym_LO->push_value(LO.get_apl_val());
   if (RO.is_function())   sym_RO->push_function(RO.get_function());
   else                    sym_RO->push_value(RO.get_apl_val());

   eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
void
UserFunction::eval_common()
{
   Log(LOG_UserFunction__enter_leave)   CERR << "eval_common()" << endl;

   if (sym_Z)   sym_Z->push();

   // push local variables...
   //
   loop(l, local_vars.size())   local_vars[l]->push();

   // push labels...
   //
   loop(l, label_values.size())
       label_values[l].sym->push_label(label_values[l].line);
}
//-----------------------------------------------------------------------------
Value_P
UserFunction::pop_local_vars() const
{
   // pop labels
   //
   loop(l, label_values.size())   label_values[l].sym->pop(true);

   // pop local variables
   //
   loop(l, local_vars.size())   local_vars[l]->pop(true);

   if (sym_B)    sym_B->pop(true);
   if (sym_X)    sym_X->pop(true);
   if (sym_A)    sym_A->pop(true);
   if (sym_LO)   sym_LO->pop(true);
   if (sym_RO)   sym_RO->pop(true);

   if (!sym_Z)   return Value_P();

   return sym_Z->pop(false);
}
//-----------------------------------------------------------------------------
void
UserFunction::set_locked_error_info(Error & error) const
{
UCS_string & message_2 = error.error_message_2;

   if (sym_A)
      {
        Value_P val_A = sym_A->get_value();
        if (!!val_A)
           {
             PrintContext pctx(PR_APL_FUN);
             PrintBuffer pb(*val_A, pctx);
             message_2.append(UCS_string(pb, 1));
             message_2.append(UNI_ASCII_SPACE);
           }
      }

   Assert(sym_FUN);
   message_2.append(sym_FUN->get_name());

   if (sym_B)
      {
        Value_P val_B = sym_B->get_value();
        if (!!val_B)
           {
             message_2.append(UNI_ASCII_SPACE);
             PrintContext pctx(PR_APL_FUN);
             PrintBuffer pb(*val_B, pctx);
             message_2.append(UCS_string(pb, 1));
           }
      }

   error.right_caret = error.left_caret + message_2.size() - 7;
}
//-----------------------------------------------------------------------------
void
UserFunction::check_duplicate_symbols()
{
   check_duplicate_symbol(sym_Z);
   check_duplicate_symbol(sym_A);
   check_duplicate_symbol(sym_LO);
   check_duplicate_symbol(sym_FUN);
   check_duplicate_symbol(sym_RO);
   check_duplicate_symbol(sym_B);

   loop(l, local_vars.size())   check_duplicate_symbol(local_vars[l]);

   loop(l, label_values.size())   check_duplicate_symbol(label_values[l].sym);
}
//-----------------------------------------------------------------------------
void UserFunction::check_duplicate_symbol(Symbol * sym)
{
   if (sym == 0)   return;   // unused symbol

int count = 0;
   if (sym == sym_Z)     ++count;
   if (sym == sym_A)     ++count;
   if (sym == sym_LO)    ++count;
   if (sym == sym_FUN)   ++count;
   if (sym == sym_RO)    ++count;
   if (sym == sym_B)     ++count;

   loop(l, local_vars.size())   if (sym == local_vars[l])   ++count;

   loop(l, label_values.size())   if (sym == label_values[l].sym)   ++count;

   if (count <= 1)   return;   // OK

   CERR << "Duplicate symbol ";
   sym->print(CERR);
   CERR << "!!!" << endl;

   DEFN_ERROR;
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::load(const char * workspace, const char * function)
{
UserFunction * fun = 0;

   try
      {
        load(workspace, function, fun);
      }
   catch (Error err)
      {
        delete fun;

        err.print(CERR);
      }
   catch (...)
      {
        delete fun;
        CERR << "Caught some exception\n";
        return 0;
      }

   return fun;
}
//-----------------------------------------------------------------------------
void
UserFunction::load(const char * workspace, const char * function,
                   UserFunction * & fun)
{
char filename[FILENAME_MAX + 10];
   snprintf(filename, FILENAME_MAX + 5,
            "workspaces/%s/%s.fun", workspace, function);

   if (strlen(filename) > FILENAME_MAX)
      {
        CERR << "file name '" << filename << "' is too long" << endl;
        throw_apl_error(E_SYSTEM_LIMIT_FILENAME, LOC);
      }

int in = open(filename, O_RDONLY);
   if (in == -1)
      {
        CERR << "Can't open() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        throw_apl_error(E_WS_OPEN, LOC);
      }

struct stat st;
   if (fstat(in, &st) == -1)
      {
        CERR << "Can't fstat() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        close(in);
        throw_apl_error(E_WS_FSTAT, LOC);
      }

off_t len = st.st_size;
void * start = mmap(0, len, PROT_READ, MAP_SHARED, in, 0);

   if (start == (const void *)-1)
      {
        CERR << "Can't mmap() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        close(in);
        throw_apl_error(E_WS_MMAP, LOC);
      }

UTF8_string utf((const UTF8 *)start, len);

   // skip trailing \r and \n.
   //
   while (len > 0 && (utf[len - 1] == '\r' || utf[len - 1] == '\n'))   --len;
   utf.erase(len);

UCS_string ucs(utf);
   munmap(start, st.st_size);
   close(in);

int error_line = -1;
   fun = fix(ucs, error_line, false, LOC);
}
//-----------------------------------------------------------------------------
Function_PC
UserFunction::pc_for_line(int l) const
{
   if (l < 1)                     return Function_PC(body.size() - 1);
   if (l >= line_starts.size())   return Function_PC(body.size() - 1);
   return line_starts[l];
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::fix(const UCS_string & text, int & error_line,
                  bool keep_existing,  const char * loc)
{
   Log(LOG_UserFunction__fix)
      CERR << "fix pmode=user function:" << endl << text << endl
           <<  "------------------- fix --" << endl;

   if (Workspace::SI_top())
      Workspace::SI_top()->set_safe_execution(true);

UserFunction * fun = 0;
   try
      {
        fun = new UserFunction(text, error_line, keep_existing, loc);
      }
   catch (Error err)
      {
        err.print(CERR);
        if (Workspace::SI_top())
           Workspace::SI_top()->set_safe_execution(false);
        return 0;
      }
   catch (...)
      {
        CERR << __FUNCTION__ << "caught something unexpected at " LOC << endl;
        if (Workspace::SI_top())
           Workspace::SI_top()->set_safe_execution(false);
        return 0;
      }

   Log(LOG_UserFunction__fix)
      {
        CERR << " addr " << (const void *)fun << endl;
        fun->print(CERR);
      }

   if (Workspace::SI_top())
      Workspace::SI_top()->set_safe_execution(false);

   // UserFunction::UserFunction() sets error line to -1 if successful.
   // We may have created a UserFunction but with an error. In this case
   // we delete it here.
   //
   if (error_line != -1)
      {
        delete fun;
        return 0;
      }

   return fun;
}
//-----------------------------------------------------------------------------
void
UserFunction::destroy()
{
   // delete will call ~Executable(), which releases the values owned by body.
   delete this;
}
//-----------------------------------------------------------------------------
/// a user-define function signature and its properties
const struct _header_pattern
{
   Fun_signature signature;      ///< a bitmap for header items
   int           symbol_count;   ///< number of header items (excl local vars)
   int           token_count;    ///< number of header token (excl local vars)
   TokenTag      tags[11];       ///< tags of the header token (excl local vars)
} header_patterns[] =            ///< all valid function headers
{
/// function result
#define __Z  TOK_LSYMB, TOK_ASSIGN

/// left function argument
#define __A  TOK_SYMBOL

/// a niladic function
#define __F0 TOK_SYMBOL

/// a monadic function
#define __F1 TOK_SYMBOL

/// a dyadic function
#define __F2 TOK_SYMBOL

/// a monadic operator
#define __OP1 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// a dyadic operator
#define __OP2 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// an axis
#define __X   TOK_L_BRACK,  TOK_SYMBOL, TOK_R_BRACK

/// right function argument
#define __B  TOK_SYMBOL

   // niladic
   //
 { SIG_F0             , 1,  1, {           __F0,            } },

 { SIG_Z_F0           , 2,  3, { __Z,      __F0,            } },

   // monadic
   //
 { SIG_F1_B           , 2,  2, {           __F1,       __B, } },
 { SIG_F1_X_B         , 3,  5, {           __F1,  __X, __B, } },
 { SIG_LO_OP1_B       , 3,  5, {           __OP1,      __B, } },
 { SIG_LO_OP1_X_B     , 4,  8, {           __OP1, __X, __B, } },
 { SIG_LO_OP2_RO_B    , 4,  6, {           __OP2,      __B, } },

 { SIG_Z_F1_B         , 3,  4, { __Z,      __F1,       __B, } },
 { SIG_Z_F1_X_B       , 4,  7, { __Z,      __F1,  __X, __B, } },
 { SIG_Z_LO_OP1_B     , 4,  7, { __Z,      __OP1,      __B, } },
 { SIG_Z_LO_OP1_X_B   , 5, 10, { __Z,      __OP1, __X, __B, } },
 { SIG_Z_LO_OP2_RO_B  , 5,  8, { __Z,      __OP2,      __B, } },

   // dyadic
   //
 { SIG_A_F2_B         , 3,  3, {      __A, __F2,       __B } },
 { SIG_A_F2_X_B       , 4,  6, {      __A, __F2,  __X, __B } },
 { SIG_A_LO_OP1_B     , 4,  6, {      __A, __OP1,      __B } },
 { SIG_A_LO_OP1_X_B   , 5,  9, {      __A, __OP1, __X, __B } },
 { SIG_A_LO_OP2_RO_B  , 5,  7, {      __A, __OP2,      __B } },

 { SIG_Z_A_F2_B       , 4,  5, { __Z, __A, __F2,       __B } },
 { SIG_Z_A_F2_X_B     , 5,  8, { __Z, __A, __F2,  __X, __B } },
 { SIG_Z_A_LO_OP1_B   , 5,  8, { __Z, __A, __OP1,      __B } },
 { SIG_Z_A_LO_OP1_X_B , 6, 11, { __Z, __A, __OP1, __X, __B } },
 { SIG_Z_A_LO_OP2_RO_B, 6,  9, { __Z, __A, __OP2,      __B } },
};

/// the number of signatures
enum { PATTERN_COUNT = sizeof(header_patterns) / sizeof(*header_patterns) };

void
UserFunction::parse_header_line(UCS_string header_line)
{
   Assert(header_line.size() > 0);

   if (header_line[header_line.size() - 1] == UNI_ASCII_CR)
      {
        header_line.shrink(header_line.size());
        Assert(header_line.size() > 0);
      }

   Log(LOG_UserFunction__set_line)
      {
        CERR << "[0] " << header_line << endl;
        // show_backtrace(__FILE__, __LINE__);
      }

   // add a semicolon as a guaranteed end marker.
   // This to avoids checks of the header token count
   // 
   header_line.append(Unicode(';'));

Token_string tos;
   {
     Parser parser(PM_FUNCTION, LOC);
     ErrorCode error = parser.parse(header_line, tos);

     if (error != E_NO_ERROR)   SYNTAX_ERROR;
   }

   // count symbols before first semicolon, allow one symbol too much.
   //
int sym_count = 0;
int tos_idx = 0;
Symbol * symbols[12];
   for (; tos_idx < 12; ++tos_idx)
      {
         if (tos[tos_idx].get_tag() == TOK_SEMICOL)   break;
         if (tos[tos_idx].get_Class() == TC_SYMBOL)
            symbols[sym_count++] = tos[tos_idx].get_sym_ptr();
      }

   // find matching signature. If sym_count or tos_idx were too high above,
   // then we will not find them in header_patterns and signal syntax error.
   //
Fun_signature signature = SIG_NONE;
   loop(s, PATTERN_COUNT)
      {
        if (header_patterns[s].symbol_count != sym_count)   continue;
        if (header_patterns[s].token_count != tos_idx)      continue;
        bool match = true;
        loop(t, tos_idx)
           {
             if (tos[t].get_tag() != header_patterns[s].tags[t])    // mismatch
                {
                   match = false;
                   break;
                }
           }

        if (match)
           {
             signature = header_patterns[s].signature;
             break;   // found signature
           }
      }

   if (signature == SIG_NONE)   SYNTAX_ERROR;

   // note: constructor has set all symbol pointers to 0!
   // store symbol pointers according to signature.
   {
     int sidx = 0;
     if (signature & SIG_Z)    sym_Z   = symbols[sidx++];
     if (signature & SIG_A)    sym_A   = symbols[sidx++];
     if (signature & SIG_LO)   sym_LO  = symbols[sidx++];
     if (signature & SIG_FUN)  sym_FUN = symbols[sidx++];
     if (signature & SIG_RO)   sym_RO  = symbols[sidx++];
     if (signature & SIG_X)    sym_X   = symbols[sidx++];
     if (signature & SIG_B)    sym_B   = symbols[sidx++];

     Assert1(sidx == sym_count);   // otherwise header_patterns is faulty
     Assert1(sym_FUN);
   }

   // set Function::tag
   //
   if      (signature & SIG_RO)   tag = TOK_OPER2;
   else if (signature & SIG_LO)   tag = TOK_OPER1;
   else if (signature & SIG_A)    tag = TOK_FUN2;
   else if (signature & SIG_B)    tag = TOK_FUN1;
   else                           tag = TOK_FUN0;

   for (;;)
      {
        if (tos[tos_idx++].get_tag() != TOK_SEMICOL)              SYNTAX_ERROR;
        if  (tos_idx >= tos.size())   break;   // local vars done

        const TokenTag tag = tos[tos_idx].get_tag();
        if (tag != TOK_SYMBOL && tag != TOK_QUAD_IO
                              && tag != TOK_QUAD_CT)
           {
             CERR << "Offending token at " LOC " is: " << tos[tos_idx] << endl;
             SYNTAX_ERROR;
           }

        local_vars.push_back(tos[tos_idx++].get_sym_ptr());
      }
}
//-----------------------------------------------------------------------------
ostream &
UserFunction::print(ostream & out) const
{
   return out << *sym_FUN;
/*
   out << "Function header:" << endl;
   if (sym_Z)     out << "Result:         " << *sym_Z   << endl;
   if (sym_A)     out << "Left Argument:  " << *sym_A   << endl;
   if (sym_LO)    out << "Left Op Arg:    " << *sym_LO  << endl;
   if (sym_FUN)   out << "Function:       " << *sym_FUN << endl;
   if (sym_RO)    out << "Right Op Arg:   " << *sym_RO  << endl;
   if (sym_B)     out << "Right Argument: " << *sym_B   << endl;
   return Executable::print(out);
*/
}
//-----------------------------------------------------------------------------
void
UserFunction::print_local_vars(ostream & out) const
{
   if (sym_Z)     out << " " << *sym_Z;
   if (sym_A)     out << " " << *sym_A;
   if (sym_LO)    out << " " << *sym_LO;
   if (sym_RO)    out << " " << *sym_RO;
   if (sym_B)     out << " " << *sym_B;

   loop(l, local_vars.size())   out << " " << *local_vars[l];
}
//-----------------------------------------------------------------------------
void
UserFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   if (is_operator())   out << "Operator " << *sym_FUN << endl;
   else                 out << "Function " << *sym_FUN << endl;

   if (get_parse_mode() == PM_FUNCTION)
      {
        if (sym_Z)    out << ind << "Result:         " << *sym_Z  << endl;
        if (sym_A)    out << ind << "Left Argument:  " << *sym_A  << endl;
        if (sym_LO)   out << ind << "Left Op Arg:    " << *sym_LO << endl;
        if (sym_RO)   out << ind << "Right Op Arg:   " << *sym_RO << endl;
        if (sym_B)    out << ind << "Right Argument: " << *sym_B  << endl;

        if (local_vars.size())
           {
                out << ind << "Local Variables:";
                loop(l, local_vars.size())
                   out << " " << *local_vars[l];
                out << endl;
           }

          out << ind << "Body Lines:     " << line_starts.size() << endl;
        if (label_values.size())
           {
                out << ind << "Labels:         ";
                loop(l, label_values.size())
                   out << " " << *label_values[l].sym
                       << " = " << label_values[l].line;
                out << endl;
           }
      }
}
//-----------------------------------------------------------------------------
const UCS_string &
UserFunction::get_name() const
{
   return sym_FUN->get_name();
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::get_name_and_line(Function_PC pc) const
{
UCS_string ret = sym_FUN->get_name();
   ret.append(UNI_ASCII_L_BRACK);

   // pc may point to the next token already. If that is the case then
   // we go back one token.
   //
   if (pc > 0 && body[pc - 1].get_Class() == TC_END)   pc = Function_PC(pc - 1);

const Function_Line line = get_line(pc);
   ret.append_number(line);
   ret.append(UNI_ASCII_R_BRACK);
   return ret;
}
//-----------------------------------------------------------------------------
Function_Line
UserFunction::get_line(Function_PC pc) const
{
   Assert(pc >= 0);
   for (int l = line_starts.size() - 1; l > 0; --l)
       {
         if (line_starts[l] <= pc)   return Function_Line(l);
       }

   return Function_Line_1;
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::canonical(bool with_lines) const
{
UCS_string ucs;
   loop(t, text.size())
      {
        if (with_lines)   ucs.append(line_prefix(Function_Line(t)));
        ucs.append(text[t]);
        ucs.append(UNI_ASCII_LF);
      }

   return ucs;
}
//-----------------------------------------------------------------------------
ostream &
UserFunction::print_val_or_fun(ostream & out, Token & tok)
{
   if (tok.is_function())         out << *tok.get_function();
   else if (tok.is_apl_val())     out << tok;
   else if (tok.is_void())        out << "((VOID))";
   else                           FIXME;

   return out;
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::line_prefix(Function_Line l)
{
UCS_string ucs;
   ucs.append(UNI_ASCII_L_BRACK);
   if (l < 10)   ucs.append(UNI_ASCII_SPACE);

char cc[40];
   snprintf(cc, sizeof(cc), "%d", l);
   for (const char * s = cc; *s; ++s)   ucs.append(Unicode(*s));

   ucs.append(UNI_ASCII_R_BRACK);
   ucs.append(UNI_ASCII_SPACE);
   return ucs;
}
//-----------------------------------------------------------------------------
