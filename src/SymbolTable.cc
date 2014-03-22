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
#include "IntCell.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "SymbolTable.hh"
#include "SystemVariable.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
SymbolTable::SymbolTable()
{
   memset(symbol_table, 0, sizeof(symbol_table));
}
//-----------------------------------------------------------------------------
NamedObject *
SymbolTable::lookup_existing_name(const UCS_string & name)
{
   Assert(!Avec::is_quad(name[0]));   // should not be called for ⎕xx

Symbol * sym = lookup_existing_symbol(name);
   if (sym == 0)   return 0;

   switch(sym->get_nc())
      {
        case NC_VARIABLE: return sym;

        case NC_FUNCTION:
        case NC_OPERATOR: return sym->get_function();
      }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
SymbolTable::lookup_existing_symbol(const UCS_string & sym_name)
{
   Assert(!Avec::is_quad(sym_name[0]));   // should not be called for ⎕xx

uint32_t hash = FNV_Offset_32;
   loop(s, sym_name.size())   hash = hash*FNV_Prime_32 ^ sym_name[s];
   hash = ((hash >> 16) ^ hash) & 0x0000FFFF;

   for (Symbol * sp = symbol_table[hash]; sp; sp = sp->next)
       {
         if (!sp->is_erased() && sp->equal(sym_name))   return sp;
       }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
SymbolTable::lookup_symbol(const UCS_string & sym_name)
{
   Assert(!Avec::is_quad(sym_name[0]));   // should not be called for ⎕xx

   // compute hash for sym_name
   //
uint32_t hash = FNV_Offset_32;
   loop(s, sym_name.size())   hash = hash*FNV_Prime_32 ^ sym_name[s];
   hash = ((hash >> 16) ^ hash) & 0x0000FFFF;

Symbol * sp = symbol_table[hash];

   if (symbol_table[hash] == 0)   // unused hash value
      {
        // sym_name is the first symbol with this hash.
        // create a new symbol, insert it into symbol_table, and return it.
        //
        Log(LOG_SYMBOL_lookup_symbol)
           {
             CERR << "Symbol " << sym_name << " has hash " << HEX(hash) << endl;
           }

        Symbol * new_symbol = new Symbol(sym_name, ID_USER_SYMBOL);
        symbol_table[hash] =  new_symbol;
        return new_symbol;
      }

   // One or more symbols with this hash already exist. The number of symbols
   // with the same name is usually very short, so we walk the list twice.
   // The first walk checks for a (possibly erased) symbol with the
   // same name and return it if found.
   //
   for (Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
       {
         if (sym->equal(sym_name))   // found
            {
              sym->set_erased(false);
              return sym;
            }
       }

   // no symbol with name sym_name exists. The second walk:
   //
   // 1. search for an erased symbol and override it, or
   // 2. append a new symbol at the end if no erased symbol was seen.
   //
int pos = 0;
   for (Symbol * sym = symbol_table[hash]; ; sym = sym->next, ++pos)
       {
         if (sym->is_erased())   // override an erased symbol.
            {
              sym->set_erased(false);
              sym->symbol = sym_name;

              Log(LOG_SYMBOL_lookup_symbol)
                 {
                   CERR << "Symbol " << sym_name << " has hash "
                        << HEX(hash) << endl;
                 }
              return sp;
            }

         if (sym->next == 0)   // append a new symbol at the end
            {
              if (pos >= 255)
                 throw_apl_error(E_SYSTEM_LIMIT_SYMTAB, LOC);

              sym->next = new Symbol(sym_name, ID_USER_SYMBOL);

              Log(LOG_SYMBOL_lookup_symbol)
                 {
                   CERR << "Symbol " << sym_name << " has hash "
                        << HEX(hash) << endl;
                 }

              return sym->next;
            }
       }

   Assert(0 && "Not reached");
}
//-----------------------------------------------------------------------------
ostream &
SymbolTable::list_symbol(ostream & out, const UCS_string & buf) const
{
Token_string tos;
      {
        Tokenizer tokenizer(PM_STATEMENT_LIST, LOC);
        if (tokenizer.tokenize(buf, tos) != E_NO_ERROR)
           {
             CERR << "parse error 1 in )SYMBOL command." << endl;
             return out;
           }
      }

   if (tos.size() == 0)   // empty argument
      {
        CERR << "parse error 2 in )SYMBOL command" << endl;
        return out;
      }

   if (tos[0].get_ValueType() != TV_SYM)
      {
        CERR << "parse error 3 in )SYMBOL command" << endl;
        return out;
      }

Symbol * symbol = tos[0].get_sym_ptr();
   return symbol->print_verbose(out);
}
//-----------------------------------------------------------------------------
void
SymbolTable::list_symbols(ostream & out, ListCategory which,
                          UCS_string from_to) const
{
UCS_string from;
UCS_string to;
   {
     const bool bad_from_to = Command::parse_from_to(from, to, from_to);
     if (bad_from_to)
        {
          CERR << "bad range argument " << from_to
               << ", expecting from - to" << endl;
          return;
        }
   }

   // put those symbols into 'list' that satisfy 'which'
   //
vector<Symbol *> list;
int symbol_count = 0;
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               ++symbol_count;

               // check range
               if (from.size() &&
                   from.compare(sym->get_name(),
                                UCS_string::upper) > 0)   continue;
               if (to.size() &&
                   to.compare(sym->get_name(),
                              UCS_string:: upper) < 0)       continue;

               if (sym->value_stack.size() == 0)
                  {
                    if (which == LIST_ALL)
                       {
                         list.push_back(sym);
                       }
                    continue;
                  }

               if (sym->is_erased() && !(which & LIST_ERASED))   continue;

               const NameClass nc = sym->value_stack.back().name_class;
               if ((nc == NC_VARIABLE)         && (which & LIST_VARS)    ||
                   (nc == NC_FUNCTION)         && (which & LIST_FUNS)    ||
                   (nc == NC_OPERATOR)         && (which & LIST_OPERS)   ||
                   (nc == NC_LABEL)            && (which & LIST_LABELS)  ||
                   (nc == NC_LABEL)            && (which & LIST_VARS)    ||
                   (nc == NC_INVALID)          && (which & LIST_INVALID) ||
                   (nc == NC_UNUSED_USER_NAME) && (which & LIST_UNUSED))
                   {
                     list.push_back(sym);
                   }
             }
       }

   if (which == LIST_NONE)   // )SYMBOLS: display total symbol count
      {
        out << "IS " << symbol_count << endl;
        return;
      }

int col = 0;
   while (list.size() > 0)
      {
        // find smallest item.
        //
        uint32_t smallest = 0;
        for (uint32_t j = 1; j < list.size(); ++j)
            {
              if (list[j]->compare(*list[smallest]) < 0)
              smallest = j;
            }

        Symbol * sym = list[smallest];
        col += sym->get_name().size();
        sym->print(out);
        if (which == LIST_NAMES)   // append .NC
           {
             col += 2;
             out << "." << sym->value_stack.back().name_class;
           }
        do out << " ";   while (++col & 7);
        if (col > 70)   { out << endl;   col = 0; }

        list[smallest] = list.back();
        list.pop_back();
      }

   out << endl;
}
//-----------------------------------------------------------------------------
void
SymbolTable::unmark_all_values() const
{
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               sym->unmark_all_values();
             }
       }
}
//-----------------------------------------------------------------------------
int
SymbolTable::show_owners(ostream & out, const Value & value) const
{
int count = 0;
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               count += sym->show_owners(out, value);
             }
       }
}
//-----------------------------------------------------------------------------
void
SymbolTable::write_all_symbols(FILE * out, uint64_t & seq) const
{
   loop(s, MAX_SYMBOL_COUNT)
       {
         for (Symbol * sym = symbol_table[s]; sym; sym = sym->next)
             {
               sym->write_OUT(out, seq);
             }
       }
}
//-----------------------------------------------------------------------------
void
SymbolTable::erase_symbols(ostream & out, const vector<UCS_string> & symbols)
{
   loop(s, symbols.size())
       {
         erase_one_symbol(out, symbols[s]);
       }
}
//-----------------------------------------------------------------------------
void
SymbolTable::clear(ostream & out)
{
   // this function should only be called when the SI is clear.
   //
   loop(hash, MAX_SYMBOL_COUNT)
      {
        Symbol * sym = symbol_table[hash];
        if (sym == 0)   continue;   // no symbol a this hash

        const bool user_defined = sym->is_user_defined();
        for (; sym; sym = sym->next)
            {
              if (sym->is_erased())               continue;
              if (sym->value_stack.size() == 0)   continue;

              Assert(!Workspace::is_called(sym->get_name()));
              Assert(sym->value_stack.size() == 1);

              sym->clear_vs();
            }

        if (user_defined)   symbol_table[hash] = 0;
      } 
}
//-----------------------------------------------------------------------------
void
SymbolTable::erase_one_symbol(ostream & out, const UCS_string & sym)
{
Symbol * symbol = lookup_existing_symbol(sym);

   if (symbol == 0)
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': unknown symbol ");
        return;
      }

   if (symbol->is_erased())
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': already erased");
        return;
      }

   if (symbol->value_stack.size() != 1)
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': symbol is pushed (localized)");
        return;
      }

   if (Workspace::is_called(sym))
      {
        UCS_string & t4 = Workspace::more_error();
        t4.clear();
        t4.append_utf8("Can't )ERASE symbol '");
        t4.append(sym);
        t4.append_utf8("': symbol is called (is on SI)");
        return;
      }

ValueStackItem & tos = symbol->value_stack[0];

   switch(tos.name_class)
      {
             case NC_LABEL:
                  Assert(0 && "should not happen since stack height == 1");
                  return;

             case NC_VARIABLE:
                  tos.name_class = NC_UNUSED_USER_NAME;
                  ptr_clear(tos.apl_val, LOC);
                  symbol->set_erased(true);
                  return;

             case NC_UNUSED_USER_NAME:
                  symbol->set_erased(true);
                  return;

             case NC_FUNCTION:
             case NC_OPERATOR:
                  Assert(tos.sym_val.function);
                  if (tos.sym_val.function->is_native())   return;

                  {
                    const UserFunction * ufun =
                                         tos.sym_val.function->get_ufun1();
                    Assert(ufun);
                    if (Workspace::oldest_exec(ufun))
                       {
                         UCS_string & t4 = Workspace::more_error();
                         t4.clear();
                         t4.append_utf8("Can't )ERASE symbol '");
                         t4.append(sym);
                         t4.append_utf8("':  pushed on SI-stack");
                         return;
                       }
                  }

                  delete tos.sym_val.function;
                  tos.sym_val.function = 0;
                  tos.name_class = NC_UNUSED_USER_NAME;
                  symbol->set_erased(true);
                  return;
      }

    Assert(0 && "Bad name_class in SymbolTable::erase_one_symbol()");
}
//-----------------------------------------------------------------------------
int
SymbolTable::symbols_allocated() const
{
int count = 0;

   loop(hash, MAX_SYMBOL_COUNT)
      {
        for (const Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
            ++count;
      }

   return count;
}
//-----------------------------------------------------------------------------
void
SymbolTable::get_all_symbols(Symbol ** table, int table_size) const
{
int idx = 0;

   loop(hash, MAX_SYMBOL_COUNT)
      {
        for (Symbol * sym = symbol_table[hash]; sym; sym = sym->next)
            {
              Assert(idx < table_size);
              table[idx++] = sym;
            }
      }
}
//-----------------------------------------------------------------------------
