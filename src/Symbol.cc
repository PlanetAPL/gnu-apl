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

#include <string.h>
#include <strings.h>

#include "CDR.hh"
#include "Command.hh"
#include "Function.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "ProcessorID.hh"
#include "QuadFunction.hh"
#include "Quad_TF.hh"
#include "Symbol.hh"
#include "Svar_signals.hh"
#include "SystemVariable.hh"
#include "UdpSocket.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Symbol::Symbol(const UCS_string & ucs, Id id)
   : NamedObject(id),
     next(0),
     symbol(ucs),
     erased(false)
{
   push();
}
//-----------------------------------------------------------------------------
ostream &
Symbol::print(ostream & out) const
{
   return out << symbol;
}
//-----------------------------------------------------------------------------
ostream &
Symbol::print_verbose(ostream & out) const
{
   out << "Symbol ";
   print(out);
   out << endl;

   loop(v, value_stack.size())
       {
         out << "[" << v << "] ";
         const ValueStackItem & item = value_stack[v];
         switch(item.name_class)
            {
              case NC_INVALID:
                   out << "---INVALID---" << endl;
                   break;

              case NC_UNUSED_USER_NAME:
                   out << "Unused user defined name" << endl;
                   break;

              case NC_LABEL:
                   out << "Label line " << item.sym_val.label << endl;
                   break;

              case NC_VARIABLE:
                   {
                      Value_P val = item.apl_val;
                      out << "Variable at " << (const void *)val.get() << endl;
                      val->print_properties(out, 8);
                      out << endl;
                   }
                   break;

              case NC_FUNCTION:
              case NC_OPERATOR:
                   {
                     Function * fun = item.sym_val.function;
                     Assert(fun);
                     fun->print_properties(out, 8);
                      out << endl;
                   }
                   continue;
            }

       }

   return out;
}
//-----------------------------------------------------------------------------
void
Symbol::assign(Value_P new_value, const char * loc)
{
   Assert(value_stack.size());

#if 0 // TODO: make this work
   if (!new_value->is_complete())
      {
        CERR << "Incomplete value at " LOC << endl;
        new_value->print_properties(CERR, 0);
        Assert(0);
      }
#endif

ValueStackItem & vs = value_stack.back();

   switch(vs.name_class)
      {
        case NC_UNUSED_USER_NAME:
             new_value = new_value->clone(loc);

             vs.name_class = NC_VARIABLE;
             vs.apl_val = new_value;
             return;

        case NC_VARIABLE:
             if (vs.apl_val == new_value)   return;   // X←X

             new_value = new_value->clone(loc);

             // un-assign and erase old value
             //
             Workspace::replace_arg(vs.apl_val, new_value);

             vs.apl_val = new_value;
             return;

        case NC_SHARED_VAR:
             {
               CDR_string cdr;
               CDR::to_CDR(cdr, new_value);
               string data((const char *)cdr.get_items(), cdr.size());
               const SV_key key = get_SV_key();

               // wait for shared variable to be ready
               //
               offered_SVAR * svar =  Svar_DB::find_var(get_SV_key());
               Assert(svar);

               for (int w = 0; ; ++w)
                   {
                     if (svar->may_set(w))   // ready for writing
                        {
                          if (w)
                             {
                               Log(LOG_shared_variables)
                                  CERR << " - OK." << endl;
                             }
                          break;
                         }

                     if (w == 0)
                        {
                          Log(LOG_shared_variables)
                             {
                               CERR << "Shared variable ";
                               svar->print_name(CERR);
                               CERR << " is blocked on set. Waiting ...";
                             }
                        }
                     else if (w%25 == 0)
                        {
                          Log(LOG_shared_variables)   CERR << ".";
                        }

                     usleep(10000);   // wait 10 ms
                   }

               // update shared var state (BEFORE sending request to peer)
               svar->set_state(false, loc);
               {
                 UdpClientSocket sock(loc, svar->data_owner_port());
                 ASSIGN_VALUE_c request(sock, get_SV_key(), data);

                 uint8_t buffer[Signal_base::get_class_size()];
                 const Signal_base * response =
                                     Signal_base::recv(sock, buffer, 10000);

                 if (response == 0)
                    {
                      cerr << "TIMEOUT on signal ASSIGN_VALUE_c" << endl;
                      VALUE_ERROR;
                    }

                 const ErrorCode ec =
                                 ErrorCode(response->get__ASSIGNED__error());
                 if (ec)
                    {
                      Log(LOG_shared_variables)
                         {
                           Error e(ec,
                                  response->get__ASSIGNED__error_loc().c_str());
                           cerr << Error::error_name(ec) << " assigning "
                                << get_name() << ", detected at "
                                << response->get__ASSIGNED__error_loc()
                                << endl;
                         }

                      throw_apl_error(ec, loc);
                    }
               }
             }
             return;
      }
             
   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
void
Symbol::assign_indexed(Value_P X, Value_P B)   // A[X] ← B
{
   // this function is called for A[X}←B when X is one-dimensional, i.e.
   // an index with no semicolons. If X contains semicolons, then
   // assign_indexed(IndexExpr IX, ...) is called instead.
   // 
Value_P A = get_apl_value();
   if (A->get_rank() != 1)   RANK_ERROR;

const ShapeItem max_idx = A->element_count();
   if (!X)   // X[] ← B
      {
        const Cell & src = B->get_ravel(0);
        loop(a, max_idx)
            {
              Cell & dest = A->get_ravel(a);
              dest.release(LOC);   // free sub-values etc (if any)
              dest.init(src);
            }
        return;
      }

const ShapeItem ec_B = B->element_count();
const ShapeItem ec_X = X->element_count();
const APL_Integer qio = Workspace::get_IO();
const APL_Float qct = Workspace::get_CT();
const int incr_B = (ec_B == 1) ? 0 : 1;   // maybe skalar extend B
const Cell * cX = &X->get_ravel(0);
const Cell * cB = &B->get_ravel(0);

   if (ec_B != 1 && ec_B != ec_X)   LENGTH_ERROR;

   loop(x, ec_X)
      {
        const ShapeItem idx = cX++->get_near_int(qct) - qio;
        if (idx < 0)          INDEX_ERROR;
        if (idx >= max_idx)   INDEX_ERROR;
        Cell & dest = A->get_ravel(idx);
        dest.release(LOC);   // free sub-values etc (if any)
        dest.init(*cB);

         cB += incr_B;
      }
}
//-----------------------------------------------------------------------------
void
Symbol::assign_indexed(const IndexExpr & IX, Value_P B)   // A[IX;...] ← B
{
   if (IX.value_count() == 1 && !!IX.values[0])   // one-dimensional index
      {
         assign_indexed(IX.values[0], B);
        return;
      }

   // see Value::index() for comments.

Value_P A = get_apl_value();
   if (A->get_rank() != IX.value_count())   INDEX_ERROR;

   // B must either B a skalar (and is then skalar extended to the size
   // of the updated area, or else have the shape of the concatenated index
   // items for example:
   //
   //  X:   X1    ; X2    ; X3
   //  ⍴B:  b1 b2   b3 b4   b5 b6
   //
   if (1 && !B->is_skalar())
      {
        // remove dimensions with len 1 from X's and B's shapes...
        // if we see an empty Xn then we return.
        //
        Shape B1;
        loop(b, B->get_rank())
           {
             const ShapeItem sb = B->get_shape_item(b);
             if (sb != 1)   B1.add_shape_item(sb);
           }

        Shape IX1;
        loop(ix, IX.value_count())
            {
              const Value * ix_val = IX.values[ix].get();
              if (ix_val)   // normal index
                 {
                   if (ix_val->element_count() == 0)   return;   // empty index
                   loop(xx, ix_val->get_rank())
                      {
                        const ShapeItem sxx = ix_val->get_shape_item(xx);
                        if (sxx != 1)   IX1.add_shape_item(sxx);
                      }
                 }
               else     // elided index: add corresponding B dimenssion
                 {
                   if (ix >= A->get_rank())   RANK_ERROR;
                   const ShapeItem sbx =
                         A->get_shape_item(A->get_rank() - ix - 1);
                   if (sbx == 0)   return;   // empty index
                   if (sbx != 1)   IX1.add_shape_item(sbx);
                 }
            }

        if (B1 != IX1)
           {
             if (B1.get_rank() != IX1.get_rank())   RANK_ERROR;
             LENGTH_ERROR;
           }
      }

MultiIndexIterator mult(A->get_shape(), IX);

const ShapeItem ec_B = B->element_count();
const Cell * cB = &B->get_ravel(0);
const int incr_B = (ec_B == 1) ? 0 : 1;

   while (!mult.done())
      {
        Cell & dest = A->get_ravel(mult.next());
        dest.release(LOC);   // free sub-values etc (if any)
        dest.init(*cB);
        cB += incr_B;
     }
}
//-----------------------------------------------------------------------------
Value_P
Symbol::pop(bool erase)
{
   Assert(value_stack.size());
const ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_VARIABLE)
      {
        Value_P ret = vs.apl_val;
        Log(LOG_SYMBOL_push_pop)
           {
             CERR << "-pop-value " << symbol;
             if (erase)   CERR  << " (and erase) ";
             CERR << " flags " << ret->get_flags() << " ";
             if (value_stack.size() == 0)   CERR << " (last)";
             CERR << " addr " << (const void *)ret.get() << endl;
           }
        if (erase)   ptr_clear(ret, LOC);

        value_stack.pop_back();
        return ret;
      }
   else
      {
        Log(LOG_SYMBOL_push_pop)
           {
             CERR << "-pop " << symbol
                  << " name_class " << vs.name_class << " ";
             if (value_stack.size() == 0)   CERR << " (last)";
             CERR << endl;
           }
        value_stack.pop_back();
        return Value_P();
      }
}
//-----------------------------------------------------------------------------
void
Symbol::push()
{
   Log(LOG_SYMBOL_push_pop)
      {
        CERR << "+push " << symbol;
        if (value_stack.size() == 0)   CERR << " (initial)";
        CERR << endl;
      }

   value_stack.push_back(ValueStackItem());
}
//-----------------------------------------------------------------------------
void
Symbol::push_label(Function_Line label)
{
   Log(LOG_SYMBOL_push_pop)
      {
        CERR << "+push_label " << symbol;
        if (value_stack.size() == 0)   CERR << " (initial)";
        CERR << endl;
      }

   value_stack.push_back(ValueStackItem(label));
}
//-----------------------------------------------------------------------------
void
Symbol::push_function(Function * function)
{
   Log(LOG_SYMBOL_push_pop)
      {
        CERR << "+push_function " << symbol << " " << (const void *)function;
        if (value_stack.size() == 0)   CERR << " (initial)";
        CERR << endl;
      }

ValueStackItem vs;
   if (function->is_operator())   vs.name_class = NC_OPERATOR;
   else                           vs.name_class = NC_FUNCTION;
   vs.sym_val.function = function;
   value_stack.push_back(vs);
}
//-----------------------------------------------------------------------------
void
Symbol::push_value(Value_P value)
{
ValueStackItem vs;
   value_stack.push_back(vs);
   assign(value, LOC);

   Log(LOG_SYMBOL_push_pop)
      {
        CERR << "+push-value " << symbol << " flags ";
        print_flags(CERR, get_value()->get_flags()) << " ";
        if (value_stack.size() == 0)   CERR << " (initial)";
        CERR << " addr " << (const void *)get_value().get() << endl;
      }
}
//-----------------------------------------------------------------------------
int
Symbol::get_ufun_depth(const UserFunction * ufun)
{
const Function * fun = (Function *)ufun;
const int sym_stack_size = value_stack_size();

   loop(s, sym_stack_size)
      {
        const ValueStackItem & vsi = (*this)[s];
        if (vsi.name_class != NC_FUNCTION)   continue;
        if (fun != vsi.sym_val.function)     continue;

       // found at level s
       //
       return s;
      }

   // not found: return -1
   return -1;
}
//-----------------------------------------------------------------------------
Value_P
Symbol::get_value()
{
const ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_VARIABLE)   return vs.apl_val;

   return Value_P();
}
//-----------------------------------------------------------------------------
bool
Symbol::can_be_defined() const
{
   if (value_stack.size() > 1)         return false;   // localized
   if (Workspace::is_called(symbol))   return false;  // on stack

   if (value_stack.back().name_class == NC_UNUSED_USER_NAME)   return true;
   if (value_stack.back().name_class == NC_FUNCTION)           return true;
   if (value_stack.back().name_class == NC_OPERATOR)           return true;
   return false;
}
//-----------------------------------------------------------------------------
Value_P
Symbol::get_apl_value() const
{
   Assert(value_stack.size() > 0);
   if (value_stack.back().name_class != NC_VARIABLE)
      throw_symbol_error(get_name(), LOC);

   return value_stack.back().apl_val;
}
//-----------------------------------------------------------------------------
bool
Symbol::can_be_assigned() const
{
   switch (value_stack.back().name_class)
      {
        case NC_UNUSED_USER_NAME:
        case NC_VARIABLE:
        case NC_SHARED_VAR:
             return true;
      }

   return false;
}
//-----------------------------------------------------------------------------
SV_key
Symbol::get_SV_key() const
{
   Assert(value_stack.size() > 0);

   if (value_stack.back().name_class != NC_SHARED_VAR)   return SV_key(0);

   return value_stack.back().sym_val.sv_key;
}
//-----------------------------------------------------------------------------
void
Symbol::set_SV_key(SV_key key)
{
   value_stack.back().name_class = NC_SHARED_VAR;
   value_stack.back().sym_val.sv_key = key;
}
//-----------------------------------------------------------------------------
const Function *
Symbol::get_function() const
{
   Assert(value_stack.size() > 0);
   switch(value_stack.back().name_class)
      {
        case NC_FUNCTION:
        case NC_OPERATOR: return value_stack.back().sym_val.function;
      }

   return 0;
}
//-----------------------------------------------------------------------------
Function *
Symbol::get_function()
{
const ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_FUNCTION)   return vs.sym_val.function;
   if (vs.name_class == NC_OPERATOR)   return vs.sym_val.function;

   return 0;
}
//-----------------------------------------------------------------------------
void
Symbol::get_attributes(int mode, Cell * dest) const
{
const ValueStackItem & vs = value_stack.back();
bool is_var = false;
int has_result = 0;
int fun_valence = 0;
int oper_valence = 0;
double created = 0.0;
static int exec_prop[] = { 0, 0, 0, 0 };
const int * exec_properties = exec_prop;

   switch(vs.name_class)
      {
        case NC_LABEL:
        case NC_VARIABLE:
             is_var = 1;
             has_result = 1;
             break;

        case NC_FUNCTION:
             has_result = vs.sym_val.function->has_result();
             fun_valence = vs.sym_val.function->get_fun_valence();
             created = vs.sym_val.function->get_creation_time();
             exec_properties = vs.sym_val.function->get_exec_properties();
             break;

        case NC_OPERATOR:
             fun_valence = vs.sym_val.function->get_fun_valence();
             oper_valence = vs.sym_val.function->get_oper_valence();
             created = vs.sym_val.function->get_creation_time();
             exec_properties = vs.sym_val.function->get_exec_properties();
             break;
      }

   switch(mode)
      {
        case 1: // valences
                new (dest + 0) IntCell(has_result);
                new (dest + 1) IntCell(fun_valence);
                new (dest + 2) IntCell(oper_valence);
                break;

        case 2: // creation time
                if (created == 0.0)   // system function
                   {
                     new (dest + 0) IntCell(0);
                     new (dest + 1) IntCell(0);
                     new (dest + 2) IntCell(0);
                     new (dest + 3) IntCell(0);
                     new (dest + 4) IntCell(0);
                     new (dest + 5) IntCell(0);
                     new (dest + 6) IntCell(0);
                   }
                else                       // user define function
                   {
                     tm tc;
                     const time_t time = created;
                     localtime_r(&time, &tc);

                     new (dest + 0) IntCell(1900 + tc.tm_year);
                     new (dest + 1) IntCell(   1 + tc.tm_mon );
                     new (dest + 2) IntCell(       tc.tm_mday);
                     new (dest + 3) IntCell(       tc.tm_hour);
                     new (dest + 4) IntCell(       tc.tm_min);
                     new (dest + 5) IntCell(       tc.tm_sec);
                     new (dest + 6) IntCell((created - time) / 1000);
                   }
                break;

        case 3: // execution properties
                new (dest + 0) IntCell(exec_properties[0]);
                new (dest + 1) IntCell(exec_properties[1]);
                new (dest + 2) IntCell(exec_properties[2]);
                new (dest + 3) IntCell(exec_properties[3]);
                break;

        case 4: {
                  Value_P val = get_apl_value();
                  const int CDR_type = val->get_CDR_type();
                  const int brutto = val->total_size_brutto(CDR_type);
                  const int data = val->data_size(CDR_type);

                  new (dest + 0) IntCell(brutto);
                  new (dest + 1) IntCell(data);
                }
                break;

        default:  Assert(0 && "bad mode");
      }
}
//-----------------------------------------------------------------------------
void
Symbol::resolve(Token & tok, bool left_sym)
{
   Log(LOG_SYMBOL_resolve)
      CERR << "resolve(" << left_sym << ") symbol " << get_name() << endl; 

   Assert1(value_stack.size());

   switch(value_stack.back().name_class)
      {
        case NC_UNUSED_USER_NAME:
             if (!left_sym)   throw_symbol_error(get_name(), LOC);
             return;   // leave symbol as is

        case NC_LABEL:
             if (left_sym)   SYNTAX_ERROR;   // assignment to (read-only) label

             {
               IntCell lab(value_stack.back().sym_val.label);
               Value_P value(new Value(lab, LOC));
               Token t(TOK_APL_VALUE1, value);
               move_1(tok, t, LOC);
             }
             return;

        case NC_VARIABLE:
             if (left_sym)   return;   // leave symbol as is

             // if we resolve a variable. the value is considered grouped.
             {
               Token t(TOK_APL_VALUE1, get_apl_value()->clone(LOC));
               move_1(tok, t, LOC);
             }
             return;

        case NC_FUNCTION:
        case NC_OPERATOR:
             move_2(tok, value_stack.back().sym_val.function->get_token(), LOC);
             return;

        case NC_SHARED_VAR:
             {
               if (left_sym)   return;   // leave symbol as is

               const SV_key key = get_SV_key();

               // wait for shared variable to be ready
               //
               offered_SVAR * svar =  Svar_DB::find_var(key);
               Assert(svar);

               for (int w = 0; ; ++w)
                   {
                     if (svar->may_use(w))   // ready for reading
                        {
                          if (w)
                             {
                               Log(LOG_shared_variables)
                                  cerr << " - OK." << endl;
                             }
                          break;
                        }

                     if (w == 0)
                        {
                          Log(LOG_shared_variables)
                             {
                               cerr << "apl" << ProcessorID::get_id().proc
                                    << ": Shared variable ";
                               svar->print_name(cerr);
                               cerr << " is blocked on use. Waiting ...";
                             }
                        }
                     else if (w%25 == 0)
                        {
                          Log(LOG_shared_variables)   cerr << ".";
                        }

                     usleep(10000);   // wait 10 ms
                   }

               UdpClientSocket sock(LOC, svar->data_owner_port());

               GET_VALUE_c request(sock, get_SV_key());
               uint8_t buffer[Signal_base::get_class_size()];
               const Signal_base * response =
                                   Signal_base::recv(sock, buffer, 10000);

               if (response == 0)
                  {
                    cerr << "TIMEOUT on signal GET_VALUE" << endl;
                    VALUE_ERROR;
                  }

               const ErrorCode err(ErrorCode(response->get__VALUE_IS__error()));
               if (err)
                  {
                    Log(LOG_shared_variables)
                       {
                         cerr << Error::error_name(err) << " referencing "
                              << get_name() << ", detected at "
                              << response->get__VALUE_IS__error_loc() << endl;
                       }

                    throw_apl_error(err, 
                                  response->get__VALUE_IS__error_loc().c_str());
                  }

               CDR_string cdr;
               const string & data = response->get__VALUE_IS__value();
               loop(d, data.size())   cdr.append(data[d]);
               Value_P value = CDR::from_CDR(cdr, LOC);

               if (!value)     VALUE_ERROR;

               // update shared var state (AFTER sending request to peer)
               {
                 offered_SVAR * svar =  Svar_DB::find_var(get_SV_key());
                 Assert(svar);
                 svar->set_state(true, LOC);
               }
               value->check_value(LOC);
               new (&tok) Token(TOK_APL_VALUE1, value);
             }
             return;

        default:
             CERR << "Symbol is '" << get_name() << "'" << endl;
             SYNTAX_ERROR;
      }
}
//-----------------------------------------------------------------------------
Token
Symbol::resolve_lv(const char * loc)
{
   Log(LOG_SYMBOL_resolve)
      CERR << "resolve_lv() symbol " << get_name() << endl; 

   Assert(value_stack.size());

   // if this is not a variable, then re-use the error handling of resolve().
   if (value_stack.back().name_class != NC_VARIABLE)
      {
        CERR << "Symbol '" << get_name()
             << "' has changed type from variable to name class "
             << value_stack.back().name_class << endl
             << " while executing an assignment" << endl;
        throw_apl_error(E_LEFT_SYNTAX_ERROR, loc);
      }

Value_P val = value_stack.back().apl_val;
   return Token(TOK_APL_VALUE1, val->get_cellrefs(loc));
}
//-----------------------------------------------------------------------------
TokenClass
Symbol::resolve_class(bool left)
{
   Assert1(value_stack.size());

   switch(value_stack.back().name_class)
      {
        case NC_LABEL:
        case NC_VARIABLE:
        case NC_SHARED_VAR:
             return (left) ? TC_SYMBOL : TC_VALUE;

        case NC_FUNCTION:
             {
               const int valence = value_stack.back().sym_val.function
                                 ->get_fun_valence();
               if (valence == 2)   return TC_FUN2;
               if (valence == 1)   return TC_FUN1;
               return TC_FUN0;
             }

        case NC_OPERATOR:
             {
               const int valence = value_stack.back().sym_val.function
                                 ->get_oper_valence();
               return (valence == 2) ? TC_OPER2 : TC_OPER1;
             }

      }

   return TC_SYMBOL;
}
//-----------------------------------------------------------------------------
int
Symbol::expunge()
{
   if (value_stack.size() == 0)   return 1;   // empty stack

ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_VARIABLE)
      {
        ptr_clear(vs.apl_val, LOC);
      }
   else if (vs.name_class == NC_FUNCTION || vs.name_class == NC_OPERATOR)
      {
        const UserFunction * ufun = vs.sym_val.function->get_ufun1();
        Assert(ufun);
        const Executable * exec = ufun;
        StateIndicator * oexec = Workspace::oldest_exec(exec);
        if (oexec)
           {
             // ufun is still used on the SI stack. We do not delete ufun,
             // but merely remember it for deletion later on.
             //
             // CERR << "⎕EX function " << ufun->get_name()
             //      << " is on SI !" << endl;
             Workspace::add_expunged_function(ufun);
           }
        else
           {
             delete ufun;
           }
      }

   vs.clear();
   return 1;
}
//-----------------------------------------------------------------------------
void
Symbol::set_nc(NameClass nc)
{
ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_UNUSED_USER_NAME)
      {
        vs.name_class = nc;
        return;
      }

   DEFN_ERROR;
}
//-----------------------------------------------------------------------------
void
Symbol::share_var(SV_key key)
{
ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_UNUSED_USER_NAME)   // new shared variable
      {
        vs.name_class = NC_SHARED_VAR;
        set_SV_key(key);
        return;
      }

   if (vs.name_class == NC_VARIABLE)           // existing variable
      {
        // remember old value
        //
        Value_P old_value = get_apl_value();

        // change name class and store AP number
        //
        vs.name_class = NC_SHARED_VAR;
        set_SV_key(key);

        // assign old value to shared variable
        assign(old_value, LOC);

        return;
      }

   DEFN_ERROR;
}
//-----------------------------------------------------------------------------
SV_Coupling
Symbol::unshare_var()
{
   if (value_stack.size() == 0)   return NO_COUPLING;

ValueStackItem & vs = value_stack.back();
   if (vs.name_class != NC_SHARED_VAR)   return NO_COUPLING;

const SV_key key = get_SV_key();
const SV_Coupling coupling = Svar_DB::retract_var(key);

   set_SV_key(0);
   vs.name_class = NC_UNUSED_USER_NAME;

   return coupling;
}
//-----------------------------------------------------------------------------
void Symbol::set_nc(NameClass nc, Function * fun)
{
ValueStackItem & vs = value_stack.back();

   if (vs.name_class == NC_UNUSED_USER_NAME)   // new function
      {
        vs.name_class = nc;
        vs.sym_val.function = fun;
        return;
      }

   if (vs.name_class == nc)   // existing function
      {
//      delete vs.sym_val.function;
        vs.sym_val.function = fun;
        return;
      }

   DEFN_ERROR;
}
//-----------------------------------------------------------------------------
ostream &
Symbol::list(ostream & out)
{
   out << "   ";
   loop(s, symbol.size())   out << symbol[s];

   for (int s = symbol.size(); s < 32; ++s)   out << " ";

   if (erased)   out << "   ERASED";
   Assert(value_stack.size());
const NameClass nc = value_stack.back().name_class;
   if      (nc == NC_INVALID)            out << "   INVALID NC";
   else if (nc == NC_UNUSED_USER_NAME)   out << "   Unused";
   else if (nc == NC_LABEL)              out << "   Label";
   else if (nc == NC_VARIABLE)           out << "   Variable";
   else if (nc == NC_FUNCTION)           out << "   Function";
   else if (nc == NC_OPERATOR)           out << "   Operator";
   else                                  out << "   !!! Error !!!";

   return out << endl;
}
//-----------------------------------------------------------------------------
void
Symbol::write_OUT(FILE * out, uint64_t & seq) const
{
const NameClass nc = value_stack[0].name_class;

char buffer[128];   // a little bigger than needed - don't use sizeof(buffer)
UCS_string data;

   switch(nc)
      {
        case NC_VARIABLE:
             {
               data.append(UNI_ASCII_A);
               data.append(get_name());
               data.append(UNI_LEFT_ARROW);
               Quad_TF::tf2_ravel(0, data, value_stack[0].apl_val);
             }
             break;

        case NC_FUNCTION:
        case NC_OPERATOR:
             {
               // write a timestamp record
               //
               Function & fun = *value_stack[0].sym_val.function;
               const YMDhmsu ymdhmsu(fun.get_creation_time());
               sprintf(buffer, "*(%d %d %d %d %d %d %d)",
                       ymdhmsu.year, ymdhmsu.month, ymdhmsu.day,
                       ymdhmsu.hour, ymdhmsu.minute, ymdhmsu.second,
                       ymdhmsu.micro/1000);

               for (char * cp = buffer + strlen(buffer);
                    cp < (buffer + 72); )   *cp++ = ' ';
                sprintf(buffer + 72, "%8.8lld\r\n", seq++);
                fwrite(buffer, 1, 82, out);

               // write function record(s)
               //
               data.append(UNI_ASCII_F);
               data.append(get_name());
               data.append(UNI_ASCII_SPACE);
               Quad_TF::tf2_fun_ucs(data, get_name(), fun);
             }
             break;

        default: return;
      }

   for (ShapeItem u = 0; u < data.size() ;)
      {
        const ShapeItem rest = data.size() - u;
        if (rest <= 71)   buffer[0] = 'X';   // last record
        else              buffer[0] = ' ';   // more records
        loop(uu, 71)
           {
             unsigned char cc = ' ';
             if (u < data.size()) cc = Command::unicode_to_cp(data[u++]);
             buffer[1 + uu] = cc;
           }

        sprintf(buffer + 72, "%8.8lld\r\n", seq++);
        fwrite(buffer, 1, 82, out);
      }
}
//-----------------------------------------------------------------------------
void
Symbol::unmark_all_values() const
{
   loop(v, value_stack.size())
       {
         const ValueStackItem & item = value_stack[v];
         switch(item.name_class)
            {
              case NC_VARIABLE:
                   item.apl_val->unmark();
                   break;

              case NC_FUNCTION:
              case NC_OPERATOR:
                   if (item.sym_val.function->is_native())   break;
                   {
                     const UserFunction * ufun =
                                          item.sym_val.function->get_ufun1();
                     Assert(ufun);

                     const Token_string & body = ufun->get_body();
                     loop(b, body.size())
                        {
                          const Token & tok = body[b];
                          if (tok.get_ValueType() == TV_VAL)
                             {
                               Value_P val = tok.get_apl_val();
                               Assert(val);
                               val->unmark();
                             }
                        }
                   }
                   break;
            }
       }
}
//-----------------------------------------------------------------------------
int
Symbol::show_owners(ostream & out, const Value & value) const
{
int count = 0;

   loop(v, value_stack.size())
       {
         const ValueStackItem & item = value_stack[v];
         switch(item.name_class)
            {
              case NC_VARIABLE:
                   if (item.apl_val.get() == &value)
                      {
                         out << "    Variable[vs=" << v << "] "
                            << get_name() << endl;
                         ++count;
                      }
                   break;

              case NC_FUNCTION:
              case NC_OPERATOR:
                   {
                     const Executable * ufun =
                                          item.sym_val.function->get_ufun1();
                     Assert(ufun);
                     char cc[100];
                     snprintf(cc, sizeof(cc), "    VS[%lld] ", v);
                     count += ufun->show_owners(cc, out, value);

                   }
                   break;
            }
       }

   return count;
}
//-----------------------------------------------------------------------------
void
Symbol::vector_assignment(vector<Symbol *> & symbols, Value_P values)
{
const int sym_count = symbols.size();
   if (values->get_rank() > 1)   RANK_ERROR;
   if (!values->is_skalar() &&
       values->element_count() != sym_count)   LENGTH_ERROR;

const int incr = values->is_skalar() ? 0 : 1;
const Cell * cV = &values->get_ravel(0);
   loop(s, sym_count)
      {
        Symbol * sym = symbols[sym_count - s - 1];
        if (cV->is_pointer_cell())
           {
             sym->assign(cV->get_pointer_value(), LOC);
           }
        else
           {
             Value_P val(new Value(LOC));
             val->get_ravel(0).init(*cV);
             sym->assign(val, LOC);
           }

        cV += incr;   // skalar extend values
      }
}
//-----------------------------------------------------------------------------
void
Symbol::clear_vs()
{
   while (value_stack.size() > 1)
      {
        Value_P deleted = pop(true);
      }

ValueStackItem & tos = value_stack[0];

   switch(tos.name_class)
      {
        case NC_LABEL:
             Assert(0 && "should not happen since stack height == 1");
             break;

        case NC_VARIABLE:
             tos.name_class = NC_UNUSED_USER_NAME;
             ptr_clear(tos.apl_val, LOC);
             break;

        case NC_UNUSED_USER_NAME:
             break;

        case NC_FUNCTION:
        case NC_OPERATOR:
             if (tos.sym_val.function->is_native())   break;

             {
               const UserFunction * ufun = tos.sym_val.function->get_ufun1();
               Assert(ufun);
               const Executable * exec = ufun;
               Assert(!Workspace::oldest_exec(exec));
             }

             delete tos.sym_val.function;
             break;
      }
}
//-----------------------------------------------------------------------------
ostream &
operator <<(ostream & out, const Symbol & sym)
{
   return sym.print(out);
}
//-----------------------------------------------------------------------------
