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

#include <iomanip>

#include "Executable.hh"
#include "IndexExpr.hh"
#include "Input.hh"
#include "Output.hh"
#include "Prefix.hh"
#include "StateIndicator.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
StateIndicator::StateIndicator(const Executable * exec, StateIndicator * _par)
   : executable(exec),
     safe_execution(false),
     eoc_handler(0),
     eoc_arg(Value_P()),
     level(_par ? 1 + _par->get_level() : 0),
     error(E_NO_ERROR, LOC),
     current_stack(*this, exec->get_body()),
     parent(_par)
{
}
//-----------------------------------------------------------------------------
StateIndicator::~StateIndicator()
{
   // flush the FIFO. Do that before delete executable so that values
   // copied directly from the body of the executable are not killed.
   //
   current_stack.cleanup();

   // some args could come from the body, so we erase them BEFORE the body.
   //
   clear_args(LOC);

   // delete the body token unless executable is a user defined function
   //
   if ((executable->get_parse_mode() != PM_FUNCTION))
      {
         Assert1(executable);
         delete executable;
         executable = 0;
      }
}
//-----------------------------------------------------------------------------
void
StateIndicator::clear_args(const char * loc)
{
   // discard duplicate args
   //
   if (eval_arg_B == eval_arg_A)   ptr_clear(eval_arg_A, loc);
   if (eval_arg_B == eval_arg_X)   ptr_clear(eval_arg_X, loc);
   if (eval_arg_A == eval_arg_X)   ptr_clear(eval_arg_X, loc);

   if (!!eval_arg_A)
      {
        ptr_clear(eval_arg_A, loc);
      }

   if (!!eval_arg_B)
      {
        ptr_clear(eval_arg_B, loc);
      }

   if (!!eval_arg_X)
      {
        ptr_clear(eval_arg_X, loc);
      }

   eval_arg_F = 0;
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_args(Token * A, Token & F, Token * X, Token * B)
{
   if (A)
      {
        eval_arg_A = A->get_apl_val();
      }

   if (X)
      {
        eval_arg_X = X->get_axes();
      }

   if (B)
      {
        eval_arg_B = B->get_apl_val();
      }

   eval_arg_F = F.get_function();
}
//-----------------------------------------------------------------------------
void
StateIndicator::goon(Function_Line new_line, const char * loc)
{
const Function_PC pc = get_executable()->get_ufun()->pc_for_line(new_line);

   Log(LOG_StateIndicator__push_pop)
      CERR << "Continue SI[" << level << "] at line " << new_line
           << " pc=" << pc << " at " << loc << endl;

   set_PC(pc);
   Log(LOG_prefix_parser)   CERR << "GOTO " << get_PC() << endl;

   current_stack.reset(LOC);
}
//-----------------------------------------------------------------------------
void
StateIndicator::retry(const char * loc)
{
   CERR << endl << "RETRY" << endl << endl;

   if (eval_arg_F == 0)
      {
        CERR << endl << "Can't RETRY: no function" << endl << endl;
        return;
      }

   if (!eval_arg_B)
      {
        CERR << endl << "Can't RETRY: no right argument" << endl << endl;
        return;
      }

Token Z;
Token B(TOK_APL_VALUE1, eval_arg_B);
int phrase_len = 1;

   if (!!eval_arg_A)   // dyadic
      {
        Token A(TOK_APL_VALUE1, eval_arg_A);
        Token F2(TOK_FUN2, eval_arg_F);

        if (!!eval_arg_X)   // dyadic with axis
           {
             Token X(TOK_APL_VALUE1, eval_arg_X);
             move_2(Z, eval_AXB(A, F2, X, B), LOC);
             phrase_len = 4;
           }
        else              // dyadic without axis
           {
             move_2(Z, eval_AB(A, F2, B), LOC);
             phrase_len = 3;
           }
      }
   else              // monadic
      {
        Token F1(TOK_FUN1, eval_arg_F);

        if (!!eval_arg_X)   // monadic with axis
           {
             Token X(TOK_APL_VALUE1, eval_arg_X);
             move_2(Z, eval_XB(F1, X, B), LOC);
             phrase_len = 3;
           }
        else              // monadic without axis
           {
             move_2(Z, eval_B(F1, B), LOC);
             phrase_len = 2;
           }
      }

   if (Z.get_Class() == TC_VALUE)   // retry successful
      {
        Token_loc tl_Z(Z, Function_PC(get_PC() - 1));
        clear_args(LOC);
        current_stack.push(tl_Z);
      }
}
//-----------------------------------------------------------------------------
UCS_string
StateIndicator::function_name() const
{
   Assert(executable);

   switch(executable->get_parse_mode())
      {
        case PM_FUNCTION:
             return executable->get_name();

        case PM_STATEMENT_LIST:
             {
               UCS_string ret;
               ret.append(UNI_DIAMOND);
               return ret;
             }

        case PM_EXECUTE:
             {
               UCS_string ret;
               ret.append(UNI_EXECUTE);
               return ret;
             }
      }

   Assert(0);
} 
//-----------------------------------------------------------------------------
void
StateIndicator::print(ostream & out) const
{
   out << "Depth:    " << level << endl;
   out << "Exec:     " << executable << endl;

   Assert(executable);

   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:
             out << "Pmode:    ∇ "
                 << executable->get_ufun()->get_name_and_line(get_PC());
             break;

        case PM_STATEMENT_LIST:
             out << "Pmode:    ◊ " << " " << executable->get_text(0);
             break;

        case PM_EXECUTE:
             out << "Pmode:    ⍎ " << " " << executable->get_text(0);
             break;

        default:
             out << "??? Bad pmode " << executable->get_parse_mode();
      }
   out << endl;

   out << "PC:       " << get_PC();
   out << " " << executable->get_body()[get_PC()] << endl;
   out << "Stat:     " << executable->statement_text(get_PC());
   out << endl;

   out << "err_code: " << HEX(error.error_code) << endl;
   out << "thrown:   at " << error.throw_loc << endl;
   out << "e_msg_1:  '" << error.get_error_line_1() << "'" << endl;
   out << "e_msg_2:  '" << error.get_error_line_2() << "'" << endl;
   out << "e_msg_3:  '" << error.get_error_line_3() << "'" << endl;

   out << endl;
}
//-----------------------------------------------------------------------------
void
StateIndicator::clear(ostream & out)
{
const UserFunction * ufun = get_executable()->get_ufun();
   if (ufun)
      {
         Value_P Z = ufun->pop_local_vars();
      }
}
//-----------------------------------------------------------------------------
void
StateIndicator::list(ostream & out, SI_mode mode) const
{
   Assert(this);

   if (mode & SIM_debug)   // command ]SI or ]SIS
      {
        print(out);
        return;
      }

   // pmode column
   //
   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:
             Assert(executable);
             if (mode == SIM_SI)   // )SI
                {
                  out << executable->get_ufun()->get_name_and_line(get_PC());
                  break;
                }

             if (mode & SIM_statements)   // )SIS
                {
                  if (error.error_code)
                     {
                       out << error.get_error_line_2() << endl
                           << error.get_error_line_3();
                     }
                  else
                     {
                       const UCS_string name_and_line =
                            executable->get_ufun()->get_name_and_line(get_PC());
                       out << name_and_line
                           << "  " << executable->statement_text(get_PC())
                           << endl
                           << UCS_string(name_and_line.size(), UNI_ASCII_SPACE)
                           << "  ^";   // ^^^
                     }
                }

             if (mode & SIM_name_list)   // )SINL
                {
                  const UCS_string name_and_line =
                        executable->get_ufun()->get_name_and_line(get_PC());
                       out << name_and_line << " ";
                       executable->get_ufun()->print_local_vars(out);
                }
             break;

        case PM_STATEMENT_LIST:
             out << "⋆";
             if (mode & SIM_statements)   // )SIS
                {
                  if (!executable)      break;

                  // )SIS and we have a statement
                  //
                  out << "  "
                      << executable->statement_text(get_PC())
                      << endl << "   ^";   // ^^^
                }
             break;

        case PM_EXECUTE:
             out << "⋆⋆  ";
             if (mode & SIM_statements)   // )SIS
                {
                  if (!executable)   break;

                  // )SIS and we have a statement
                  //
                  if (error.error_code)
                     out << error.get_error_line_2() << endl
                         << error.get_error_line_3();
                  else
                     out << "  "
                         << executable->statement_text(get_PC());
                }
             break;
      }

   out << endl;
}
//-----------------------------------------------------------------------------
void
StateIndicator::update_error_info(Error & err)
{
bool locked = false;
const UserFunction * ufun = executable->get_ufun();

   // prepare second error line (failed statement)
   //
   if (ufun)
      {
        if (err.show_locked || ufun->get_exec_properties()[1])
           {
             locked = true;
             err.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
           }
        else
           {
             err.error_message_2 = ufun->get_name_and_line(get_PC());
             err.error_message_2.append(UNI_ASCII_SPACE);
             err.error_message_2.append(UNI_ASCII_SPACE);
           }
      }
   else
      {
        err.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
      }

   // prepare third line (carets)
   //
   err.left_caret = err.error_message_2.size();
   err.right_caret = -1;

   if (locked)
      {
        executable->get_ufun()->set_locked_error_info(err);
      }
   else
      {
        const Function_PC from = current_stack.get_range_low();
        Function_PC to   = current_stack.get_range_high();
        const Function_PC2 error_range(from, to);
        // current_stack.print_range(CERR);
        // CERR  << endl;
        executable->set_error_info(err, error_range);
      }

   // print error, unless we are in safe execution mode.
   //
   if (!safe_execution)   err.print_em(UERR, LOC);

   Workspace::update_EM_ET(err);   // update ⎕EM and ⎕ET

   error = err;
}
//-----------------------------------------------------------------------------
ostream &
StateIndicator::indent(ostream & out) const
{
   if (level < 0)
      {
         CERR << "[negative level " << HEX(level) << "]" << endl;
      }
   else if (level > 100)
      {
         CERR << "[huge level " << HEX(level) << "]" << endl;
      }
   else
      {
         loop(d, level)   out << "   ";
      }

   return out;
}
//-----------------------------------------------------------------------------
Token
StateIndicator::jump(Value_P value)
{
   // perform a jump. We either remain in the current function (and then
   // return TOK_VOID), or we (want to) jump into back into the calling
   // function (and then return TOK_BRANCH.). The jump itself (if any)
   // is executed in Command.cc.
   //
   if (value->get_rank() > 1)   DOMAIN_ERROR;

   if (value->element_count() == 0)     // →''
      {
        // →'' in immediate execution means resume (retry) suspended function
        if (get_executable()->get_parse_mode() == PM_STATEMENT_LIST)
           return Token(TOK_BRANCH, int64_t(Function_Retry));

        return Token(TOK_VOID);           // stay in context
      }

const Function_Line line = value->get_line_number(Workspace::get_CT());

const UserFunction * ufun = get_executable()->get_ufun();

   if (ufun)   // →N in user defined function
      {
        set_PC(ufun->pc_for_line(line));   // →N to valid line in user function
        return Token(TOK_VOID);         // stay in context
      }

   // →N in ⍎ or ◊
   //
   return Token(TOK_BRANCH, int64_t(line < 0 ? Function_Line_0 : line));
}
//-----------------------------------------------------------------------------
void
StateIndicator::escape()  
{
const UserFunction * ufun = get_executable()->get_ufun();
   if (ufun)
      {
        Value_P Z = ufun->pop_local_vars();
      }
}
//-----------------------------------------------------------------------------
Token
StateIndicator::run()
{
Token result = current_stack.reduce_statements();

   Log(LOG_prefix_parser)
      CERR << "Prefix::reduce_statements(si=" << level << ") returned "
           << result << " in StateIndicator::run()" << endl;
   return result;
}
//-----------------------------------------------------------------------------
void
StateIndicator::unmark_all_values() const
{
   if (!!eval_arg_A)   eval_arg_A->clear_marked();
   if (!!eval_arg_X)   eval_arg_X->clear_marked();
   if (!!eval_arg_B)   eval_arg_B->clear_marked();

   Assert(executable);
   executable->unmark_all_values();

   current_stack.unmark_all_values();
}
//-----------------------------------------------------------------------------
int
StateIndicator::show_owners(ostream & out, const Value & value) const
{
int count = 0;

   if (&value == eval_arg_A.get())
      { out << "    SI[" << level << "] eval_arg_A" << endl;   ++count; }

   if (&value == eval_arg_X.get())
      { out << "    SI[" << level << "] eval_arg_X" << endl;   ++count; }

   if (&value == eval_arg_B.get())
      { out << "    SI[" << level << "] eval_arg_B" << endl;   ++count; }

   Assert(executable);
char cc[100];
   snprintf(cc, sizeof(cc), "    SI[%d] ", level);
   count += executable->show_owners(cc, out, value);

   snprintf(cc, sizeof(cc), "    SI[%d] ", level);
   current_stack.show_owners(cc, out, value);

   return count;
}
//-----------------------------------------------------------------------------
void
StateIndicator::info(ostream & out, const char * loc) const
{
   out << "SI[" << level << ":" << get_PC() << "] "
       << get_parse_mode_name() << " "
       << executable->get_text(0) << " creator: " << executable->get_loc()
       << "   seen at: " << loc << endl;
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_L() const
{
   if (eval_arg_F == 0)                 return Value_P();   // no function
   if (eval_arg_F->is_user_defined())   return Value_P();   // user defined function
   return eval_arg_A;
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_L(Value_P new_value)
{
Value_P old_value = eval_arg_A;

   eval_arg_A = new_value;
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_X() const
{
   if (eval_arg_F == 0)                 return Value_P();   // no function
   if (eval_arg_F->is_user_defined())   return Value_P();   // user defined function
   return eval_arg_X;
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_X(Value_P new_value)
{
Value_P old_value = eval_arg_X;

   eval_arg_X = new_value;
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_R() const
{
   if (eval_arg_F == 0)                 return Value_P();   // no function
   if (eval_arg_F->is_user_defined())   return Value_P();   // user defined function
   return eval_arg_B;
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_R(Value_P new_value)
{
Value_P old_value = eval_arg_B;

   eval_arg_B = new_value;
}
//-----------------------------------------------------------------------------
bool
StateIndicator::replace_arg(Value_P old_value, Value_P new_value)
{
   current_stack.replace_AB(old_value, new_value);

bool success = false;
   if (old_value == eval_arg_A)
      {
        eval_arg_A = new_value;
        success = true;
      }

   if (old_value == eval_arg_B)
      {
        eval_arg_B = new_value;
        success = true;
      }

   if (old_value == eval_arg_X)
      {
        eval_arg_X = new_value;
        success = true;
      }

   return success;
}
//-----------------------------------------------------------------------------
void
StateIndicator::recover_from_error(const char * loc)
{
   Value::finish_incomplete(loc);
}
//-----------------------------------------------------------------------------
/// a macro for the common part of all StateIndicator::eval_XXX() calls
/// catch exceptions and check for errors,
#define FINISH_EVAL \
   catch (Error e)                              \
      {                                         \
       recover_from_error(LOC);                 \
       return Token(TOK_ERROR, e.error_code);   \
      }                                         \
   catch (...)                                  \
      {                                         \
        FIXME;                                  \
      }                                         \
                                                \
   return Token(TOK_ERROR, error.error_code);

//-----------------------------------------------------------------------------
Token
StateIndicator::eval_(Token & fun)
{
   set_args(0, fun, 0, 0);

   try
      {
        const Token result = eval_arg_F->eval_();

        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_B(Token & fun, Token & B)
{
   set_args(0, fun, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_B(eval_arg_B);

        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_AB(Token & A, Token & fun, Token & B)
{
   set_args(&A, fun, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_AB(eval_arg_A, eval_arg_B);

        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_XB(Token & fun, Token & X, Token & B)
{
   set_args(0, fun, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_XB(eval_arg_X,
                                                 eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_AXB(Token & A, Token & fun, Token & X, Token & B)
{
   set_args(&A, fun, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_AXB(eval_arg_A,
                                                  eval_arg_X,
                                                  eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_LB(Token & LO, Token & oper, Token & B)
{
   set_args(0, oper, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_LB(LO, eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_ALB(Token & A, Token & LO, Token & oper, Token & B)
{
   set_args(&A, oper, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_ALB(eval_arg_A, LO, eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_LXB(Token & LO, Token & oper, Token & X, Token & B)
{
   set_args(0, oper, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_LXB(LO, eval_arg_X, eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_LRB(Token & LO, Token & oper, Token & RO, Token & B)
{
   set_args(0, oper, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_LRB(LO, RO, eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_ALXB(Token & A, Token & LO, Token & oper,
                          Token & X, Token & B)
{
   set_args(&A, oper, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_ALXB(eval_arg_A, LO, eval_arg_X,
                                                   eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_ALRB(Token & A, Token & LO, Token & oper,
                          Token & RO, Token & B)
{
   set_args(&A, oper, 0, &B);

   try
      {
        const Token result = eval_arg_F->eval_ALRB(eval_arg_A, LO, RO,
                                                   eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_LRXB(Token & LO, Token & oper,
                          Token & X, Token & RO, Token & B)
{
   set_args(0, oper, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_LRXB(LO, RO, eval_arg_X,
                                                   eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Token
StateIndicator::eval_ALRXB(Token & A, Token & LO, Token & oper,
                           Token & X, Token & RO, Token & B)
{
   set_args(&A, oper, &X, &B);

   try
      {
        const Token result = eval_arg_F->eval_ALRXB(eval_arg_A, LO, RO,
                                                    eval_arg_X,
                                                    eval_arg_B);
        if (result.get_tag() != TOK_ERROR)
           {
             clear_args(LOC);
             return result;
           }
      }

   FINISH_EVAL
}
//-----------------------------------------------------------------------------
Function_Line
StateIndicator::get_line() const
{
   return executable->get_line(Function_PC(get_PC() - 1));
}
//-----------------------------------------------------------------------------
void
StateIndicator::statement_result(Token & result)
{
   Log(LOG_StateIndicator__enter_leave)
      CERR << "StateIndicator::statement_result(pmode="
           << get_parse_mode_name() << ", result=" << result << endl;

   fun_oper_cache.reset();

   if (get_executable()->get_parse_mode() == PM_EXECUTE)   return;

   // if result is a value then print it, unless it is a committed value
   // (i.e. TOK_APL_VALUE2)
   //
   if (result.get_ValueType() == TV_VAL)
      {
        const TokenTag tag = result.get_tag();
        Value_P B(result.get_apl_val());
        Assert(!!B);

        // print TOK_APL_VALUE and TOK_APL_VALUE1, but not TOK_APL_VALUE2
        //
        if (tag == TOK_APL_VALUE1 || tag == TOK_APL_VALUE3)
           {
             Quad_QUOTE::done(false, LOC);
             B->print(COUT);
           }
      }
}
//-----------------------------------------------------------------------------
Unicode
StateIndicator::get_parse_mode_name() const
{
   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:       return UNI_NABLA;
        case PM_STATEMENT_LIST: return UNI_DIAMOND;
        case PM_EXECUTE:        return UNI_EXECUTE;
     }

   CERR << "pmode = " << get_executable()->get_parse_mode() << endl;
   Assert(0 && "bad pmode");
}
//-----------------------------------------------------------------------------

