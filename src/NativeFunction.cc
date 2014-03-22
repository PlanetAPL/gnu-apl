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


#include <dlfcn.h>

#include "Error.hh"
#include "NativeFunction.hh"
#include "Symbol.hh"
#include "Workspace.hh"

vector<NativeFunction *> NativeFunction::valid_functions;

//-----------------------------------------------------------------------------
NativeFunction::NativeFunction(const UCS_string & so_name,
                               const UCS_string & apl_name)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     handle(0),
     name(apl_name),
     so_path(so_name),
     valid(false)
{
   // open .so file...
   //
   {
     UTF8_string lib_name(so_path);
     handle = 0;

     if ((strchr(lib_name.c_str(), '/') == 0) &&
         (strchr(lib_name.c_str(), '\\') == 0))
        {
          // the lib_name contains no path prefix. Try:
          //
          // PKGLIBDIR/lib_name
          // PKGLIBDIR/lib_name.so
          // PKGLIBDIR/lib_name.dylib
          //
          UTF8_string pkg_path = PKGLIBDIR;
          pkg_path.append('/');
          pkg_path.append(lib_name);
          handle = dlopen(pkg_path.c_str(), RTLD_NOW);
          if (handle == 0)   // no luck: try pkg_path.so
             {
               UTF8_string pkg_path__so(pkg_path);
               pkg_path__so.append(UTF8_string(".so"));
               handle = dlopen(pkg_path__so.c_str(), RTLD_NOW);
             }
          if (handle == 0)   // still no luck: try pkg_path.dylib
             {
               UTF8_string pkg_path__dylib(pkg_path);
               pkg_path__dylib.append(UTF8_string(".dylib"));
               handle = dlopen(pkg_path__dylib.c_str(), RTLD_NOW);
             }
        }

     if (handle == 0)   // lib_name was not in PKGLIBDIR.
        {
          // Try:
          //
          // lib_name
          // lib_name.so
          // lib_name.dylib

          handle = dlopen(lib_name.c_str(), RTLD_NOW);
          if (handle == 0)   // no luck: try lib_name.so
             {
               UTF8_string lib_name__so(lib_name);
               lib_name__so.append(UTF8_string(".so"));
               handle = dlopen(lib_name__so.c_str(), RTLD_NOW);
             }
          if (handle == 0)   // still no luck: try lib_name.dylib
             {
               UTF8_string lib_name__dylib(lib_name);
               lib_name__dylib.append(UTF8_string(".dylib"));
               handle = dlopen(lib_name__dylib.c_str(), RTLD_NOW);
             }
        }

     if (handle == 0)   // all dlopen() failed
        {
          Workspace::more_error() = UCS_string(dlerror());
          return;
        }
   }

   // get the function multiplexer
   //
void * fmux = dlsym(handle, "get_function_mux");
     if (!fmux)
        {
          CERR << "shared library is lacking the mandatory "
                  "function get_function_mux() !" << endl;
          Workspace::more_error() = UCS_string(
                                  "invalid .so file (no get_function_mux())");
          return;
        }

void * (*get_function_mux)(const char *) = (void * (*)(const char *))fmux;

   // get the mandatory function category
   //
   // check for mandatory properties in shared linrary...
   //
   {
     void * get_sig = get_function_mux("get_signature");
     if (!get_sig)
        {
          CERR << "shared library is lacking the mandatory "
                  "function signature() !" << endl;
        Workspace::more_error() = UCS_string(
                                "invalid .so file (no get_signature())");
          return;
        }

     signature = ((Fun_signature (*)())get_sig)();
   }


   // create an entry in the symbol table
   //
Symbol * sym = Workspace::lookup_symbol(apl_name);
   Assert(sym);
   if (!sym->can_be_defined())
      {
        Workspace::more_error() = UCS_string("Symbol is locked");
        return;
      }

   /// read function pointers...
   //
#define ev(fun, args) f_ ## fun = (Token (*) args ) get_function_mux(#fun)

   ev(eval_        , ()                              );

   ev(eval_B       , (Vr B)                          );
   ev(eval_AB      , (Vr A, Vr B)                    );
   ev(eval_XB      , (Vr X, Vr B)                    );
   ev(eval_AXB     , (Vr A, Vr X, Vr B)              );

   ev(eval_LB      , (Fr LO, Vr B)                   );
   ev(eval_ALB     , (Vr A, Fr LO, Vr B)             );
   ev(eval_LXB     , (Fr LO, Vr X, Vr B)             );
   ev(eval_ALXB    , (Vr A, Fr LO, Vr X, Vr B)       );

   ev(eval_LRB     , (Fr LO, Fr RO, Vr B)            );
   ev(eval_LRXB    , (Fr LO, Fr RO, Vr X, Vr B)      );
   ev(eval_ALRB    , (Vr A, Fr LO, Fr RO, Vr B)      );
   ev(eval_ALRXB   , (Vr A, Fr LO, Fr RO, Vr X, Vr B));

   ev(eval_fill_B  , (Vr B)                          );
   ev(eval_fill_AB , (Vr A, Vr B)                    );
   ev(eval_ident_Bx, (Vr B, Axis x)                  );
#undef ev

   // compute function tag based on the signature
   //
   if      (signature & SIG_RO)   tag = TOK_OPER2;
   else if (signature & SIG_LO)   tag = TOK_OPER1;
   else if (signature & SIG_B )   tag = TOK_FUN2;
   else                           tag = TOK_FUN0;

   if (is_operator())   sym->set_nc(NC_OPERATOR, this);
   else                 sym->set_nc(NC_FUNCTION, this);

   valid = true;
   valid_functions.push_back(this);
}
//-----------------------------------------------------------------------------
NativeFunction::~NativeFunction()
{
   if (handle)   dlclose(handle);

   loop(v, valid_functions.size())
      {
        if (valid_functions[v] == this)
           {
             valid_functions.erase(valid_functions.begin() + v);
           }
      }
}
//-----------------------------------------------------------------------------
NativeFunction *
NativeFunction::fix(const UCS_string & so_name,
                    const UCS_string & function_name)
{
   // if the function already exists then return it.
   //
   loop(v, valid_functions.size())
      {
        if (so_name == valid_functions[v]->so_path)   return valid_functions[v];
      }

NativeFunction * new_function = new NativeFunction(so_name, function_name);

   if (!new_function->valid)   // something went wrong
      {
        delete new_function;
        return 0;
      }

   return new_function;
}
//-----------------------------------------------------------------------------
int
NativeFunction::has_result() const
{
   return !!(signature & SIG_Z);
}
//-----------------------------------------------------------------------------
bool
NativeFunction::is_operator() const
{
   // this function is an operator if one of the eval functions with a
   // function argument is present...
   //
   return f_eval_LB
       || f_eval_ALB
       || f_eval_LRB
       || f_eval_ALRB
       || f_eval_LXB
       || f_eval_ALXB
       || f_eval_LRXB
       || f_eval_ALRXB;
}
//-----------------------------------------------------------------------------
void
NativeFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Native Function " << endl;
}
//-----------------------------------------------------------------------------
UCS_string
NativeFunction::canonical(bool with_lines) const
{
   return so_path;
}
//-----------------------------------------------------------------------------
ostream &
NativeFunction::print(std::ostream & out) const
{
   if (is_operator())   out << "native Operator " << name << endl;
   else                 out << "native Function " << name << endl;

   return out;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_()
{
   if (f_eval_)   return (*f_eval_)();

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_B(Value_P B)
{
   if (f_eval_B)   return (*f_eval_B)(B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AB(Value_P A, Value_P B)
{
   if (f_eval_AB)   return (*f_eval_AB)(A, B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LB(Token & LO, Value_P B)
{
   if (f_eval_LB)   return (*f_eval_LB)(*LO.get_function(), B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   if (f_eval_ALB)   return (*f_eval_ALB)(A, *LO.get_function(), B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   if (f_eval_LRB)   return (*f_eval_LRB)(*LO.get_function(),
                                          *RO.get_function(), B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   if (f_eval_ALRB)   return (*f_eval_ALRB)(A, *LO.get_function(),
                                               *RO.get_function(), B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_XB(Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_XB)   return (*f_eval_XB)(X, B);
   else             return eval_B(B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_AXB)   return (*f_eval_AXB)(A, X, B);
   else              return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LXB)   return (*f_eval_LXB)(*LO.get_function(), X, B);
   else              return eval_LB(LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALXB)   return (*f_eval_ALXB)(A, *LO.get_function(), X, B);
   else               return eval_ALB(A, LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LRXB)   return (*f_eval_LRXB)(*LO.get_function(),
                                            *RO.get_function(), X, B);
   else                return eval_LRB(LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALRXB)   return (*f_eval_ALRXB)(A, *LO.get_function(),
                                                 *RO.get_function(), X, B);
   else                return eval_ALRB(A, LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_B(Value_P B)
{
   if (f_eval_fill_B)   return (*f_eval_fill_B)(B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_AB(Value_P A, Value_P B)
{
   if (f_eval_fill_AB)   return (*f_eval_fill_AB)(A, B);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_identity_fun(Value_P B, Axis axis)
{
   if (f_eval_ident_Bx)   return (*f_eval_ident_Bx)(B, axis);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------

