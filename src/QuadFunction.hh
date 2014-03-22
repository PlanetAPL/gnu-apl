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

#ifndef __QUAD_FUNCTION_HH_DEFINED__
#define __QUAD_FUNCTION_HH_DEFINED__

#include "PrimitiveFunction.hh"
#include "StateIndicator.hh"

//-----------------------------------------------------------------------------
/** The various Quad functions.
 */
class QuadFunction : public PrimitiveFunction
{
public:
   /// Constructor.
   QuadFunction(TokenTag tag) : PrimitiveFunction(tag) {}

   /// overloaded Function::has_alpha()
   virtual bool has_alpha() const   { return true; }

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B) { VALENCE_ERROR; }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B) { VALENCE_ERROR; }

   /// overloaded Function::has_result()
   virtual int has_result() const   { return 1; }
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-AF (Atomic Function)
 */
class Quad_AF : public QuadFunction
{
public:
   /// Constructor.
   Quad_AF() : QuadFunction(TOK_QUAD_AF) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   static Quad_AF           fun;          ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-AT (Attributes)
 */
class Quad_AT : public QuadFunction
{
public:
   /// Constructor.
   Quad_AT() : QuadFunction(TOK_QUAD_AT) {}

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   static Quad_AT fun;          ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-CR (Character Representation).
 */
class Quad_CR : public QuadFunction
{
public:
   /// Constructor.
   Quad_CR() : QuadFunction(TOK_QUAD_CR) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// compute a ⎕CR B
   Value_P do_CR(APL_Integer a, const Value & B);

   static Quad_CR fun;          ///< Built-in function.

protected:
   /// 10 ⎕CR symbol_name (variable ir function name)
   void do_CR10(vector<UCS_string> & result, const Value & symbol_name);

   /// portable variable encoding of value named name (varname or varname ⊂)
   void do_CR10_var(vector<UCS_string> & result, const UCS_string & name,
                    const UCS_string & pick, const Value & value);

};
//-----------------------------------------------------------------------------
/**
   The system function Quad-DL (Delay).
 */
class Quad_DL : public QuadFunction
{
public:
   /// Constructor.
   Quad_DL() : QuadFunction(TOK_QUAD_DL) {}

   static Quad_DL fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-EA (Execute Alternate)
 */
class Quad_EA : public QuadFunction
{
public:
   /// Constructor.
   Quad_EA() : QuadFunction(TOK_QUAD_EA) {}

   static Quad_EA fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// end of context handler for ⎕EA
   static bool eoc_B_done(Token & token, EOC_arg & arg);

   /// end of context handler for ⎕EA
   static bool eoc_A_done(Token & token, EOC_arg & arg);

   /// update ⎕EM
   static void update_EM(Value_P A, Value_P B, bool A_failed);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-EC (Execute Controlled)
 */
class Quad_EC : public QuadFunction
{
public:
   /// Constructor.
   Quad_EC() : QuadFunction(TOK_QUAD_EC) {}

   static Quad_EC           fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// end of context handler for ⎕EC
   static bool eoc(Token & token, EOC_arg & arg);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-ENV (ENvironment Variables)
 */
class Quad_ENV : public QuadFunction
{
public:
   /// Constructor.
   Quad_ENV() : QuadFunction(TOK_QUAD_ENV) {}

   static Quad_ENV fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-ES (Event Simulate).
 */
class Quad_ES : public QuadFunction
{
public:
   /// Constructor.
   Quad_ES() : QuadFunction(TOK_QUAD_ES) {}

   static Quad_ES           fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// common inplementation for eval_AB() and eval_B()
   Token event_simulate(const UCS_string * A, Value_P B, Error & error);

   /// compute error code for B
   static ErrorCode get_error_code(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-EX (Expunge).
 */
class Quad_EX : public QuadFunction
{
public:
   /// Constructor.
   Quad_EX() : QuadFunction(TOK_QUAD_EX) {}

   static Quad_EX fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// disassociate name from value, return 0 on failure or 1 on success.
   int expunge(UCS_string name);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-INP (input from script, aka. HERE document)
 */
class Quad_INP : public QuadFunction
{
public:
   /// Constructor.
   Quad_INP() : QuadFunction(TOK_QUAD_INP) {}

   static Quad_INP fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// read input until end_marker seen; maybe ⍎ esc1...esc2.
   static bool eoc_INP(Token & token, EOC_arg & arg);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-NA (Name Association).
 */
class Quad_NA : public QuadFunction
{
public:
   /// Constructor.
   Quad_NA() : QuadFunction(TOK_QUAD_NA) {}

   static Quad_NA fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)   { TODO; }

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)   { TODO; }
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-NC (Name class).
 */
class Quad_NC : public QuadFunction
{
public:
   /// Constructor.
   Quad_NC() : QuadFunction(TOK_QUAD_NC) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   static Quad_NC           fun;          ///< Built-in function.

protected:
   /// return the ⎕NC for variable name \b var
   APL_Integer get_NC(const UCS_string var);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-NL (Name List).
 */
class Quad_NL : public QuadFunction
{
public:
   /// Constructor.
   Quad_NL() : QuadFunction(TOK_QUAD_NL) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return do_quad_NL(Value_P(), B); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return do_quad_NL(A, B); }

   static Quad_NL           fun;          ///< Built-in function.

protected:
   /// return A ⎕NL B
   Token do_quad_NL(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-SI (State Indicator)
 */
class Quad_SI : public QuadFunction
{
public:
   /// Constructor.
   Quad_SI() : QuadFunction(TOK_QUAD_SI) {}

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);

   /// Built-in function.
   static Quad_SI fun;

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function Quad-UCS (Universal Character Set)
 */
class Quad_UCS : public QuadFunction
{
public:
   /// Constructor.
   Quad_UCS() : QuadFunction(TOK_QUAD_UCS) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// Built-in function.
   static Quad_UCS fun;

protected:
};
//-----------------------------------------------------------------------------

#endif // __QUAD_FUNCTION_HH_DEFINED__
