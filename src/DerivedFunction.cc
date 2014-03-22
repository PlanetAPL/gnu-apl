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
#include "DerivedFunction.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "StateIndicator.hh"
#include "Workspace.hh"

//=============================================================================
DerivedFunction::DerivedFunction(Token & lfun, Function * dyop, Token & rfun, const char * loc)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     left_fun(lfun),
     oper(dyop),
     right_fun(rfun),
     axis(Value_P())
{
   Assert1(oper);

   Log(LOG_FunOperX)
      {
        print(CERR<< "DerivedFunction(dyadic with 2 functions)");
        CERR << " at " << loc << endl;
     }
}
//-----------------------------------------------------------------------------
DerivedFunction::DerivedFunction(Token & lfun, Function * monop, const char * loc)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     left_fun(lfun),
     oper(monop),
     right_fun(TOK_VOID),
     axis(Value_P())
{
   Assert1(oper);

   Log(LOG_FunOperX)
      {
        print(CERR<< "DerivedFunction(monadic)");
        CERR << " at " << loc << endl;
     }
}
//-----------------------------------------------------------------------------
DerivedFunction::DerivedFunction(Token & lfun, Function * monop, Value_P X, const char * loc)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     left_fun(lfun),
     oper(monop),
     right_fun(TOK_VOID),
     axis(X)
{
   Assert1(oper);

   Log(LOG_FunOperX)
      {
        print(CERR<< "DerivedFunction(monadic with axis)");
        CERR << " at " << loc << endl;
     }
}
//-----------------------------------------------------------------------------
Token
DerivedFunction::eval_B(Value_P B)
{
   Log(LOG_FunOperX)
      {
        print(CERR << "entering DerivedFunction");
        CERR << "::eval_B() , this = " << (void *)this << endl;
      }

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
        return oper->eval_LRB(left_fun, right_fun, B);
      }
   else                                   // monadic operator
      {
        if (!!axis)   return oper->eval_LXB(left_fun, axis, B);
        else          return oper->eval_LB(left_fun, B);
      }
}
//-----------------------------------------------------------------------------
Token
DerivedFunction::eval_XB(Value_P X, Value_P B)
{
   Log(LOG_FunOperX)
      {
        print(CERR << "entering DerivedFunction");
        CERR << "::eval_XB() , this = " << (void *)this << endl;
      }

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
        return oper->eval_LRXB(left_fun, right_fun, X, B);
      }
   else                                   // monadic operator
      {
        return oper->eval_LXB(left_fun, X, B);
      }
}
//-----------------------------------------------------------------------------
Token
DerivedFunction::eval_AB(Value_P A, Value_P B)
{
   Log(LOG_FunOperX)
      {
        print(CERR << "entering DerivedFunction");
        CERR << "::eval_AB()" << endl;
      }

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
        return oper->eval_ALRB(A, left_fun, right_fun, B);
      }
   else                                   // monadic operator
      {
        if (!!axis)     return oper->eval_ALXB(A, left_fun, axis, B);
        else            return oper->eval_ALB(A, left_fun, B);
      }
}
//-----------------------------------------------------------------------------
Token
DerivedFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   Log(LOG_FunOperX)
      {
        print(CERR << "entering DerivedFunction");
        CERR << "::eval_AXB()" << endl;
      }

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
        return oper->eval_ALRXB(A, left_fun, right_fun, X, B);
      }
   else                                   // monadic operator
      {
        return oper->eval_ALXB(A, left_fun, X, B);
      }
}
//-----------------------------------------------------------------------------
ostream &
DerivedFunction::print(ostream & out) const
{
   out << "(";
   if (left_fun.is_function())   left_fun.get_function()->print(out);
   else                          out << "VAL";
   out << " ";

   oper->print(out);
   if (!!axis)   out << "[]";

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
        out << " ";
        if (right_fun.is_function())   right_fun.get_function()->print(out);
        else                           out << "VAL";
      }

   return out << ")";
}
//-----------------------------------------------------------------------------
void
DerivedFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Function derived from operator" << endl
       << ind << "Left Function: ";
   if (left_fun.is_function())   left_fun.get_function()->print(out);
   else                          out << "VAL";
   out << endl << ind << "Operator:  ";
   oper->print(out);
   if (!!axis)   out << "Axis: " << *axis << endl;

   if (right_fun.get_tag() != TOK_VOID)   // dyadic operator
      {
         out << ind << "Right Function:  ";
        if (right_fun.is_function())   right_fun.get_function()->print(out);
        else                           out << "VAL";
      }

   out << endl;
}
//=============================================================================
DerivedFunctionCache::DerivedFunctionCache()
   : idx(0)
{
   Log(LOG_FunOperX)
      {
         CERR << "DerivedFunctionCache created, cache at " << (const void *)cache
              << "..." << (const void *)(cache + MAX_FUN_OPER) << endl;
      }
}
//-----------------------------------------------------------------------------
DerivedFunctionCache::~DerivedFunctionCache()
{
   Log(LOG_FunOperX)
      {
         CERR << "DerivedFunctionCache deleted, cache at " << (const void *)cache
              << "..." << (const void *)(cache + MAX_FUN_OPER) << endl;
      }
}
//-----------------------------------------------------------------------------
void
DerivedFunctionCache::reset()
{
   idx = 0;

   Log(LOG_FunOperX)
      {
         CERR << "DerivedFunctionCache reset, cache at " << (const void *)cache
              << "..." << (const void *)(cache + MAX_FUN_OPER) << endl;
      }
}
//-----------------------------------------------------------------------------
DerivedFunction *
DerivedFunctionCache::get(const char * loc)
{
   if (idx >= MAX_FUN_OPER)   LIMIT_ERROR_FUNOPER;

   Log(LOG_FunOperX)
      {
         CERR << "DerivedFunctionCache get( " << idx << " ), cache at "
              << (const void *)cache << "..."
              << (const void *)(cache + MAX_FUN_OPER)
              << " at " << loc << endl;
      }

   return cache + idx++;
}
//=============================================================================

