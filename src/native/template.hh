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
#include <errno.h>
#include <string.h>

#include "../Native_interface.hh"

/**
   This file demonstrates how to write native APL functions. A native
   APL function is a function that is written in C++ but can be called
   in APL like a user-defined APL function.
   
   In GNU APL a native function is created by using a variant of dyadic ⎕FX:

	soname ⎕FX funname

   soname is the path to a shared library
   funname is the name of the function

   For example:

   'native/.libs/lib_file_io.so' ⎕FX 'FILE_IO'
FILE_IO

   creates a native function FILE_IO. FILE_IO can then be called (the
   convention for FILE_IO is that the axis is a sub-function number (and
   sub-function 1 returns the error string for a numeric error code provided
   as right argument for FILE_IO):

      FILE_IO[1] 0   ⍝  error string for error code 0
Success

      FILE_IO[1] 1   ⍝  error string for error code 1
Operation not permitted

      FILE_IO[1] 2   ⍝  error string for error code 2
No such file or directory

      FILE_IO[1] 3   ⍝  error string for error code 3
No such process

   It is up to the native function to decide which function signatures
   it implements (and not implemented signatures will throw a SYNTAX ERROR).
   In the case of FILE_IO, the signatures Z←A FILE_IO[X] B and Z←FILE_IO[X] B
   i.e. monadic and dyadic function with axis were implemented.

   The native function implementated in this file is a template for your
   own native function. It implements all function signatures supported by
   GNU APL and simply returns ia character vector telling how it was called.

**/

// mandatory functios
extern "C" void * get_function_mux(const char * function_name);
static Fun_signature get_signature();
static Token eval_fill_B(Value_P B);
static Token eval_fill_AB(Value_P A, Value_P B);
static Token eval_ident_Bx(Value_P B, Axis x);

#if defined TEMPLATE_F0

Fun_signature get_signature() { return SIG_Z_F0; }

static Token eval_();

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "eval_"))           return (void *)&eval_;
   if (!strcmp(function_name, "eval_fill_B"))     return (void *)&eval_fill_B;
   if (!strcmp(function_name, "eval_fill_AB"))    return (void *)&eval_fill_AB;
   if (!strcmp(function_name, "eval_ident_Bx"))   return (void *)&eval_ident_Bx;
   return 0;
}

Token
eval_()
{
UCS_string ucs("eval_() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

#elif defined TEMPLATE_F12

Fun_signature get_signature() { return SIG_Z_A_F2_B; }

static Token eval_B(Value_P B);
static Token eval_AB(Value_P A, Value_P B);
static Token eval_XB(Value_P X, Value_P B);
static Token eval_AXB(Value_P A, Value_P X, Value_P B);

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "eval_B"))          return (void *)&eval_B;
   if (!strcmp(function_name, "eval_AB"))         return (void *)&eval_AB;
   if (!strcmp(function_name, "eval_XB"))         return (void *)&eval_XB;
   if (!strcmp(function_name, "eval_AXB"))        return (void *)&eval_AXB;
   if (!strcmp(function_name, "eval_fill_B"))     return (void *)&eval_fill_B;
   if (!strcmp(function_name, "eval_fill_AB"))    return (void *)&eval_fill_AB;
   if (!strcmp(function_name, "eval_ident_Bx"))   return (void *)&eval_ident_Bx;
   return 0;
}
//-----------------------------------------------------------------------------
Token
eval_B(Value_P B)
{
UCS_string ucs("eval_B() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B)
{
UCS_string ucs("eval_AB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B)
{
UCS_string ucs("eval_XB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_AXB(Value_P A, Value_P X, Value_P B)
{
UCS_string ucs("eval_AXB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

#elif defined TEMPLATE_OP1

Fun_signature get_signature() { return SIG_Z_A_LO_OP1_B; }

static Token eval_LB(Function & LO, Value_P B);
static Token eval_ALB(Value_P A, Function & LO, Value_P B);
static Token eval_LXB(Function & LO, Value_P X, Value_P B);
static Token eval_ALXB(Value_P A, Function & LO, Value_P X, Value_P B);

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "eval_LB"))         return (void *)&eval_LB;
   if (!strcmp(function_name, "eval_ALB"))        return (void *)&eval_ALB;
   if (!strcmp(function_name, "eval_LXB"))        return (void *)&eval_LXB;
   if (!strcmp(function_name, "eval_ALXB"))       return (void *)&eval_ALXB;
   if (!strcmp(function_name, "eval_fill_B"))     return (void *)&eval_fill_B;
   if (!strcmp(function_name, "eval_fill_AB"))    return (void *)&eval_fill_AB;
   if (!strcmp(function_name, "eval_ident_Bx"))   return (void *)&eval_ident_Bx;
   return 0;
}
//-----------------------------------------------------------------------------
Token
eval_LB(Function & LO, Value_P B)
{
UCS_string ucs("eval_LB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_ALB(Value_P A, Function & LO, Value_P B)
{
UCS_string ucs("eval_ALB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_LXB(Function & LO, Value_P X, Value_P B)
{
UCS_string ucs("eval_LXB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_ALXB(Value_P A, Function & LO, Value_P X, Value_P B)
{
UCS_string ucs("eval_ALXB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

#elif defined TEMPLATE_OP2

Fun_signature get_signature() { return SIG_Z_A_LO_OP2_RO_B; }

static Token eval_LRB(Function & LO, Function & RO, Value_P B);
static Token eval_ALRB(Value_P A, Function & LO, Function & RO, Value_P B);
static Token eval_LRXB(Function & LO, Function & RO, Value_P X, Value_P B);
static Token eval_ALRXB(Value_P A, Function & LO, Function & RO, Value_P X, Value_P B);

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "eval_LRB"))        return (void *)&eval_LRB;
   if (!strcmp(function_name, "eval_ALRB"))       return (void *)&eval_ALRB;
   if (!strcmp(function_name, "eval_LRXB"))       return (void *)&eval_LRXB;
   if (!strcmp(function_name, "eval_ALRXB"))      return (void *)&eval_ALRXB;
   if (!strcmp(function_name, "eval_fill_B"))     return (void *)&eval_fill_B;
   if (!strcmp(function_name, "eval_fill_AB"))    return (void *)&eval_fill_AB;
   if (!strcmp(function_name, "eval_ident_Bx"))   return (void *)&eval_ident_Bx;
   return 0;
}
//-----------------------------------------------------------------------------
Token
eval_LRB(Function & LO, Function & RO, Value_P B)
{
UCS_string ucs("eval_LRB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_ALRB(Value_P A, Function & LO, Function & RO, Value_P B)
{
UCS_string ucs("eval_ALRB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}

//-----------------------------------------------------------------------------
Token
eval_LRXB(Function & LO, Function & RO, Value_P X, Value_P B)
{
UCS_string ucs("eval_LRXB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_ALRXB(Value_P A, Function & LO, Function & RO, Value_P X, Value_P B)
{
UCS_string ucs("eval_ALRXB() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

#endif

Token
eval_fill_B(Value_P B)
{
UCS_string ucs("eval_fill_B() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_fill_AB(Value_P A, Value_P B)
{
UCS_string ucs("eval_fill_B() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_ident_Bx(Value_P B, Axis x)
{
UCS_string ucs("eval_ident_Bx() called");
Value_P Z(new Value(ucs, LOC));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

