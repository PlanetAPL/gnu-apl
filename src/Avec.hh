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

#ifndef __AVEC_HH_DEFINED__
#define __AVEC_HH_DEFINED__

#include "Common.hh"

class Token;


/// The valid indices the Atomic Vector of the APL interpreter
enum CHT_Index
{
   Invalid_CHT = -1,
#define char_def( n, _u, _t, _f, _p) AV_ ## n,
#define char_df1(_n, _u, _t, _f, _p)
#include "Avec.def"
   MAX_AV,
};

///  Some flags helping to classify characters
enum CharacterFlag
{
   FLG_NONE            = 0x0000,   ///< no flag
   FLG_SYMBOL          = 0x0001,   ///< valid char in a user defined name
   FLG_DIGIT           = 0x0002,   ///< 0-9
   FLG_NO_SPACE_AFTER  = 0x0004,   ///< never need a space after this char
   FLG_NO_SPACE_BEFORE = 0x0008,   ///< never need a space before this char

   FLG_NO_SPACE        = FLG_NO_SPACE_AFTER | FLG_NO_SPACE_BEFORE,
   FLG_NUMERIC         = FLG_DIGIT | FLG_SYMBOL,   ///< 0-9, A-Z, a-z, ∆, ⍙
};

/**
    class Avec is a collection of static functions related to the Atomic
    Vector of the APL interpreter
 */
class Avec
{
public:
   /// init the static tables of this class and check them
   static void init();

   /// Return the UNICODE of char table entry \b av
   static Unicode unicode(CHT_Index av);

   /// Return the UNICODE of char table entry \b av
   static uint32_t get_av_pos(CHT_Index av);

   /// Return a token containing \b av
   static Token uni_to_token(Unicode av, const char * loc);

   /// Return \b true iff \b av is a valid char in a user defined symbol
   static bool is_symbol_char(Unicode av);

   /// Return \b true iff \b av is a valid char in a user defined symbol
   static bool is_first_symbol_char(Unicode uni);

   /// Return \b true iff \b av is a digit (.i.e. 0...9)
   static bool is_digit(Unicode uni);

   /// Return \b true iff \b av is a digit or a space
   static bool is_digit_or_space(Unicode uni);

   /// return \b true iff \b av is a number char (digit, .)
   static bool is_number(Unicode uni);

   /// return true if unicode \b is defined by a char_def() or char_df1() macro
   static bool is_known_char(Unicode uni);

   /// return true if unicode \b is a quad (⎕ or ▯)
   static bool is_quad(Unicode uni)
      { return uni == UNI_QUAD_QUAD || uni == UNI_QUAD_QUAD1; }

   /// return \b true iff a token printed after \b av never needs a space
   static bool no_space_after(Unicode uni);

   /// return \b true iff a token printed before \b av never needs a space
   static bool no_space_before(Unicode av);

   /// return the subscript char for digit i
   static Unicode subscript(uint32_t i);

   /// return the superscript char for digit i
   static Unicode superscript(uint32_t i);

   /// Find \b av in \b character_table. Return AV position if found or MAX_AV
   static uint32_t find_av_pos(Unicode av);

   /// Find \b av in \b character_table. Return position or Invalid_CHT
   static CHT_Index find_char(Unicode av);

protected:
   /// Find \b return position of \b alt_av, or Invalid_AV if not found
   static CHT_Index map_alternative_char(Unicode alt_av);

   /// print an error position on cerr, and then Assert(0);
   static void show_error_pos(int i, int line, bool cond, int def_line);

   /// check that the character table that is used in this class is correct
   static void check_av_table();

   /// check that all characters in the UTF-8 encoded file are known
   /// (through char_def() or char_df1() macros)
   static void check_file(const char * filename);
};

#endif // __AVEC_HH_DEFINED__
