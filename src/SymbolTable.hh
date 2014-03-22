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

#ifndef __SYMBOLTABLE_HH_DEFINED__
#define __SYMBOLTABLE_HH_DEFINED__

#include <stdint.h>

class Symbol;

//-----------------------------------------------------------------------------
/**
     A table containing all user defined symbols.
 */
class SymbolTable
{
public:
   /// Construct an empty \b SymbolTable.
   SymbolTable();

   /// Return or create a \b Symbol with name \b ucs in \b this \b SymbolTable.
   Symbol * lookup_symbol(const UCS_string & ucs);

   /// Return a \b Symbol with name \b ucs in \b this \b SymbolTable.
   NamedObject * lookup_existing_name(const UCS_string & name);

   /// Return a \b Symbol with name \b ucs in \b this \b SymbolTable.
   Symbol * lookup_existing_symbol(const UCS_string & ucs);

   /// List all symbols in \b this \b SymbolTable (for )VARS, )FNS etc.)
   void list_symbols(ostream & out, ListCategory which,
                     UCS_string from_to) const;

   /// clear the marked flag of all symbols
   void unmark_all_values() const;

   /// print variables owning value
   int show_owners(ostream & out, const Value & value) const;

   /// clear this symbol table (remove all symbols)
   void clear(ostream & out);

   /// erase symbols from \b this SymbolTable
   void erase_symbols(ostream & out, const vector<UCS_string> & symbols);

   /// List details of single symbol in buf.
   ostream & list_symbol(ostream & out, const UCS_string & buf) const;

   /// write all symbols in )OUT format to file \b out
   void write_all_symbols(FILE * out, uint64_t & seq) const;

   /// Number of symbols in \b this \b SymbolTable (including erased symbols)
   int symbols_allocated() const;

   /// return all symbols  (including erased symbols)
   void get_all_symbols(Symbol ** table, int table_size) const;

protected:
   /// Parameters for the FNV-1 hash.
   enum { FNV_Offset_32 = 0x811C9DC5, FNV_Prime_32 = 16777619 };

   /// Hash table for all symbols.
   Symbol * symbol_table[MAX_SYMBOL_COUNT];

   /// dito
   void erase_one_symbol(ostream & out, const UCS_string & sym);
};
//-----------------------------------------------------------------------------

#endif // __SYMBOLTABLE_HH_DEFINED__
