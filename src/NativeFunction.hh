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

#ifndef __NATIVE_FUNCTION_HH_DEFINED__
#define __NATIVE_FUNCTION_HH_DEFINED__

#include <vector>
#include <sys/types.h>

#include "Executable.hh"
#include "Function.hh"
#include "Error.hh"

//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class NativeFunction : public Function
{
public:
   /// ⎕FX a native funtion from a shared library (dlopen) handle
   static NativeFunction * fix(const UCS_string & so_name,
                               const UCS_string & apl_name);

   /// kind of native function
   enum NativeCategory
      {
        NCAT_F0,   ///< niladic function
        NCAT_F12,   ///< nomadic (monadic or dyadic) function
        NCAT_OP1,   ///< monadic operator
        NCAT_OP2,   ///< dyadic operator
      };

protected:
   /// constructor (only fix() may call it)
   NativeFunction(const UCS_string & so_name, const UCS_string & apl_name);

   /// destructor: close so_handle
   ~NativeFunction();

   /// overloaded Function::is_native()
   virtual bool is_native() const   { return true; }

   /// overloaded Function::has_result()
   virtual int has_result() const;

   /// overloaded Function::is_operator()
   virtual bool is_operator() const;

   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// Overloaded Function::print()
   virtual ostream & print(std::ostream&) const;

   /// Overloaded Function::canonical()
   virtual UCS_string canonical(bool with_lines) const;

   /// Overloaded Function::eval_()
   virtual Token eval_();

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_LB()
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_ALB()
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Overloaded Function::eval_LXB()
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALXB()
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X,
                            Value_P B);

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_identity_fun()
   virtual Token eval_identity_fun(Value_P B, Axis axis);

   /// Overloaded Function::destroy()
   virtual void destroy()
      { delete this; }

   /// dl_open() handle of shared library
   void * handle;

   /// APL name of the function
   UCS_string name;

   /// library file path of the function
   UCS_string so_path;

   /// true if the shared lib was successfully opened
   bool valid;

   /// type of function (niladic / nomadic / moadic operator / dyadic operator)
   Fun_signature signature;

   static vector<NativeFunction *>valid_functions;

   typedef Value_P Vr;
   typedef Function & Fr;

   Token (*f_eval_)        (                              );
   Token (*f_eval_B)       (                          Vr B);
   Token (*f_eval_AB)      (Vr A,                     Vr B);
   Token (*f_eval_LB)      (      Fr LO,              Vr B);
   Token (*f_eval_ALB)     (Vr A, Fr LO,              Vr B);
   Token (*f_eval_LRB)     (      Fr LO, Fr RO,       Vr B);
   Token (*f_eval_ALRB)    (Vr A, Fr LO, Fr RO,       Vr B);
   Token (*f_eval_XB)      (                    Vr X, Vr B);
   Token (*f_eval_AXB)     (Vr A,               Vr X, Vr B);
   Token (*f_eval_LXB)     (      Fr LO,        Vr X, Vr B);
   Token (*f_eval_ALXB)    (Vr A, Fr LO,        Vr X, Vr B);
   Token (*f_eval_LRXB)    (      Fr LO, Fr RO, Vr X, Vr B);
   Token (*f_eval_ALRXB)   (Vr A, Fr LO, Fr RO, Vr X, Vr B);

   Token (*f_eval_fill_B)  (                          Vr B);
   Token (*f_eval_fill_AB) (Vr A,                     Vr B);
   Token (*f_eval_ident_Bx)(Vr B,                   Axis x);
};
//-----------------------------------------------------------------------------

#endif // __NATIVE_FUNCTION_HH_DEFINED__
