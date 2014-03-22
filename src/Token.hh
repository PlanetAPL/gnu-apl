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

#ifndef __TOKEN_HH_DEFINED__
#define __TOKEN_HH_DEFINED__

#include <ostream>

#include "Common.hh"
#include "Avec.hh"
#include "Id.hh"
#include "Error.hh"
#include "Simple_string.hh"
#include "Source.hh"
#include "TokenEnums.hh"

class Function;
class IndexExpr;
class Symbol;
class Value;
class Workspace;

/**
    A Token, consisting of a \b tag and a \b value. The \b tag (actually
    already (tag & TV_MASK) identifies the type of the \b value.
 */
class Token
{
public:
   /// Construct a VOID token. VOID token are used for two purposes: (1) to
   /// fill positions when e.g. 3 tokens were replaced by 2 tokens during
   /// parsing, and (2) as return values of user defined functions that
   /// do not return values.
   Token()
   : tag(TOK_VOID) { value.int_val = 0; }

   /// copy constructor
   Token(const Token & other);

   /// Construct a token without a value
   Token(TokenTag tg)
   : tag(tg) { Assert(get_ValueType() == TV_NONE);   value.int_val = 0; }

   /// Construct a token for a \b Function.
   Token(TokenTag tg, Function * fun)
   : tag(tg) { Assert(get_ValueType() == TV_FUN);   value.function = fun; }

   /// Construct a token for a \b line number
   Token(TokenTag tg, Function_Line line)
   : tag(tg) { Assert(tg == TOK_LINE);   value.fun_line = line; }

   /// Construct a token for an \b error code
   Token(TokenTag tg, ErrorCode ec)
   : tag(tg) { Assert(tg == TOK_ERROR);   value.int_val = ec; }

   /// Construct a token for a \b Symbol
   Token(TokenTag tg, Symbol * sp)
   : tag(tg) { Assert(get_ValueType() == TV_SYM);  value.sym_ptr = sp; }

   /// Construct a token with tag tg for a UNICODE character. The tag is
   /// defined in Avec.def. This token in temporary in the sense that
   /// get_ValueType() can be anything rather than TV_CHAR. The value
   /// for the token (if any) will be added later (after parsing it).
   Token(TokenTag tg, Unicode uni)
   : tag(tg) { value.char_val = uni; }

   /// Construct a token for a single integer value.
   Token(TokenTag tg, int64_t ival)
   : tag(tg) { value.int_val = ival; }

   /// Construct a token for a single floating point value.
   Token(TokenTag tg, double flt)
   : tag(tg) { value.flt_val = flt; }

   /// Construct a token for a single complex value.
   Token(TokenTag tg, double r, double i)
   : tag(tg)
     {
       value.complex_val.real = r;
       value.complex_val.imag = i;
     }

   /// Construct a token for an APL value.
   Token(TokenTag tg, Value_P vp)
   : tag(tg)
   { Assert1(get_ValueType() == TV_VAL);
     Assert(!!vp);   new (&value._apl_val()) Value_P(vp); }

   /// Construct a token for an index
   Token(TokenTag tg, IndexExpr & idx);

   ~Token()
     { extract_apl_val("~Token()");  }

   /// return the TokenValueType of this token.
   TokenValueType get_ValueType() const
      { return TokenValueType(tag & TV_MASK); }

   /// return the TokenClass of this token.
   TokenClass get_Class() const
      { return TokenClass(tag & TC_MASK); }

   /// return the Id of this token.
   Id get_Id() const
      { return Id(tag >> 16); }

   /// return the tag of this token
   const TokenTag get_tag() const   { return tag; }

   /// return the Unicode value of this token
   Unicode get_char_val() const
      { Assert(get_ValueType() == TV_CHAR);   return value.char_val; }

   /// return the integer value of this token
   int64_t get_int_val() const
      { Assert(get_ValueType() == TV_INT);   return value.int_val; }

   /// set the integer value of this token
   void set_int_val(int64_t val)
      { Assert(get_ValueType() == TV_INT);   value.int_val = val; }

   /// return the float value of this token
   APL_Float get_flt_val() const
      { Assert(get_ValueType() == TV_FLT);   return value.flt_val; }

   /// return the complex real value of this token
   double get_cpx_real() const
      { Assert(get_ValueType() == TV_CPX);   return value.complex_val.real; }

   /// return the complex imag value of this token
   double get_cpx_imag() const
      { Assert(get_ValueType() == TV_CPX);   return value.complex_val.imag; }

   /// return the Symbol * value of this token
   Symbol * get_sym_ptr() const
      { Assert(get_ValueType() == TV_SYM);   return value.sym_ptr; }

   /// return the Function_Line value of this token
   Function_Line get_fun_line() const
      { Assert(get_ValueType() == TV_LIN);   return value.fun_line; }

   /// return true iff \b this token has no value
   bool is_void() const
      { return (get_ValueType() == TV_NONE); }

   /// return true iff \b this token is an apl value
   bool is_apl_val() const
      { return (get_ValueType() == TV_VAL); }

   /// return the Value_P value of this token. The token could be TOK_NO_VALUE;
   /// in that case VALUE_ERROR is thrown.
   Value_P get_apl_val() const
      { if (!is_apl_val())   VALUE_ERROR;   return value._apl_val(); }

   /// return the Value * of this token, or 0 for non-value token
   Value * get_apl_val_pointer() const
      { return is_apl_val() ? value._apl_val().get() : 0; }

   /// return value usage counter
   int value_use_count() const
      { if (!is_apl_val())   return 0;
        if (!value._apl_val())   return -98;
        return value._apl_val().use_count();
      }

   /// clear the Value_P value (if any) this token, return the old pointer
   void extract_apl_val(const char * loc) const
      { if (is_apl_val())   ptr_clear(value._apl_val(), loc); }

   void clear(const char * loc)
      {
         if (is_apl_val())   ptr_clear(value._apl_val(), loc);
         new (this) Token();
      }

   /// return the axis specification of this token (expect non-zero axes)
   Value_P get_nonzero_axes() const
      { Assert1(!!value._apl_val() && (get_tag() == TOK_AXES));
        return value._apl_val(); }

   /// return the axis specification of this token
   Value_P get_axes() const
      { Assert1(get_tag() == TOK_AXES);  return value._apl_val(); }

   /// set the Value_P value of this token
   void set_apl_val(Value_P val)
      { Assert(get_ValueType() == TV_VAL);   value._apl_val() = val; }

   /// return the IndexExpr value of this token
   IndexExpr & get_index_val() const
      { Assert(get_ValueType() == TV_INDEX);   return *value.index_val; }

   /// return true if \b this token is a function (or operator)
   bool is_function() const
      { return (get_ValueType() == TV_FUN); }

   /// return the Function * value of this token
   Function * get_function() const
      { if (!is_function())   SYNTAX_ERROR;   return value.function; }

   /// change the tag (within the same TokenValueType)
   void ChangeTag(TokenTag new_tag);

   /// helper function to print a function.
   ostream & print_function(ostream & out) const;

   /// helper function to print an APL value
   ostream & print_value(ostream & out) const;

   /// the Quad_CR representation of the token.
   UCS_string canonical(PrintStyle style) const;

   /// the tag in readable form (TOK_...)
   UCS_string tag_name() const;

   /// print the token to \b out in the format used by print_error_info().
   /// return the number of characters printed.
   int error_info(UCS_string & out) const;

   /// copy src to \b this token
   void copy_N(const Token & src);

   /// return a brief token class name for debugging purposes
   static const char * short_class_name(TokenClass cls);

   /// print a list of token
   static void print_token_list(ostream & out, const Source<Token> & list);

   /// the optional value of the token.
   union sval
      {
        ///< a complex number (for TV_CPX)
        struct cdouble
           {
              APL_Float real;   ///< the real part
              APL_Float imag;   ///< the imaginary part
           };

        Unicode                char_val;      ///< the Unicode for CTV_CHARTV_
        int64_t                int_val;       ///< the integer for TV_INT
        APL_Float              flt_val;       ///< the double for TV_FLT
        cdouble                complex_val;   ///< complex number for TV_CPX
        Symbol               * sym_ptr;        ///< the symbol for TV_SYM
        Function_Line          fun_line;      ///< the function line for TV_LIN
        IndexExpr            * index_val;     ///< the index for TV_INDEX
        Function             * function;      ///< the function for TV_FUN
        VALUE_P(               apl_val)       ///< the value for TV_VAL
      };

   /// the name of \b tc
   static const char * class_name(TokenClass tc);

protected:
   /// The tag indicating the type of \b this token
   TokenTag tag;

   /// The value of \b this token
   sval value;

   /// helper function to print Quad-function (system function or variable).
   ostream & print_quad(ostream & out) const;

private:
   // prevent accidental copying
   Token & operator =(const Token & other);
};
//-----------------------------------------------------------------------------
/// A string of Token
class Token_string : public  Simple_string<Token>
{
public:
   /// construct an empty string
   Token_string() 
   : Simple_string<Token>()
   {}

   /// construct a string of \b len Token, starting at \b data.
   Token_string(const Token * data, uint32_t len)
   : Simple_string<Token>(data, len)
   {}

   /// construct a string of \b len Token from another token string
   Token_string(const Token_string & other, uint32_t pos, uint32_t len)
   : Simple_string<Token>(other, pos, len)
   {}

private:
   // prevent accidental copying
   Token_string & operator =(const Token_string & other);
};
//-----------------------------------------------------------------------------
/// a token with its location information. For token copied from a function
/// body: low = high = PC. For token from a reduction low is the low location
/// of the first token and high is the high of the last token of the token
/// range that led to e.g a result token.
struct Token_loc
{
   /// constructor: invalid Token_loc
   Token_loc()
   : pc(Function_PC_invalid)
   {}

   /// constructor: invalid Token with valid loc
   Token_loc(Function_PC _pc)
   : pc(_pc)
   {}

   /// constructor: valid Token with valid loc
   Token_loc(const Token & t, Function_PC _pc)
   : tok(t),
     pc(_pc)
   {}

   void copy(const Token_loc & other, const char * loc)
      {
        pc = other.pc;
        copy_1(tok, other.tok, loc);
      }

   /// the token
   Token tok;

   /// the PC of the leftmost (highest PC) token
   Function_PC pc;
};
//-----------------------------------------------------------------------------

#endif // __TOKEN_HH_DEFINED__
