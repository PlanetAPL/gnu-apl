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

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Avec.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Value.hh"

using namespace std;

// See RFC 3629 for UTF-8

/// Various character attributes.
struct Character_definition
{
   CHT_Index     av_val;      ///< Atomic vector enum of the char.
   Unicode       unicode;     ///< Unicode of the char.
   const char *  char_name;   ///< Name of the char.
   int           def_line;    ///< Line where the char is defined.
   TokenTag      token_tag;   ///< Token tag for the char.
   CharacterFlag flags;       ///< Character class.
   int           av_pos;      ///< position in ⎕AV (== ⎕AF unicode)
};

//-----------------------------------------------------------------------------
#define char_def(n, _u, t, f, p) \
   { AV_ ## n, UNI_ ## n, # n, __LINE__, TOK_ ## t, FLG_ ## f, 0x ## p },
#define char_df1(_n, _u, _t, _f, _p)
const Character_definition character_table[MAX_AV] =
{
#include "Avec.def"
};
//-----------------------------------------------------------------------------
void
Avec::show_error_pos(int i, int line, bool cond, int def_line)
{
   if (!cond)
      {
        CERR << "Error index (line " << line << ", Avec.def line "
             << def_line << ") = " << i << " = " << HEX(i) << endl;
        Assert(0);
      }
}
//-----------------------------------------------------------------------------
#if 0

void
Avec::check_file(const char * filename)
{
const int fd = open(filename, O_RDONLY);
   if (fd == -1)   return;

   Log(LOG_startup)   CERR << "Checking " << filename << endl;

uint32_t datalen;
   {
     struct stat s;
     const int res = fstat(fd, &s);
     Assert(res == 0);
     datalen = s.st_size;
   }

const void * data = mmap(NULL, datalen, PROT_READ, MAP_PRIVATE, fd, 0);
   Assert(data != (void *)-1);

const UTF8_string utf = UTF8_string((const UTF8 *)data, datalen);
UCS_string ucs(utf);

   loop(i, ucs.size())
       {
         if (!is_known_char(ucs[i]))
            CERR << "APL char " << UNI(ucs[i]) << " is missing in AV ("
                 << i << ")" << endl;
       }

   close(fd);
}
#endif
//-----------------------------------------------------------------------------
void
Avec::init()
{
   check_av_table();
}
//-----------------------------------------------------------------------------
void
Avec::check_av_table()
{
   if (MAX_AV != 256)
      {
        CERR << "AV has " << MAX_AV << " entries (should be 256)" << endl;
        return;
      }

   Assert(sizeof(character_table) / sizeof(*character_table) == MAX_AV);

   // check that character_table.unicode is sorted by increasing UNICODE
   //
   for (int i = 1; i < MAX_AV; ++i)
       show_error_pos(i, __LINE__, character_table[i].def_line,
          character_table[i].unicode > character_table[i - 1].unicode);

   // check that ASCII chars map to themselves.
   //
   loop(i, 0x80)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                  character_table[i].unicode == i);

   // display holes and duplicate AV positions in character_table
   {
     int holes = 0;
     loop(pos, MAX_AV)
        {
          int count = 0;
          loop(i, MAX_AV)
              {
                if (character_table[i].av_pos == pos)   ++count;
              }

          if (count == 0)
             {
               ++holes;
               CERR << "AV position " << HEX(pos) << " unused" << endl;
             }

          if (count > 1)
             CERR << "duplicate AV position " << HEX(pos) << endl;
        }

     if (holes)   CERR << holes << " unused positions in ⎕AV" << endl;
   }

   // check that find_char() works
   //
   loop(i, MAX_AV)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                    i == find_char(character_table[i].unicode));

   Log(LOG_SHOW_AV)
      {
        for (int i = 0x80; i < MAX_AV; ++i)
            {
              CERR << character_table[i].unicode << " is AV[" << i << "] AV_"
                   << character_table[i].char_name << endl;
            }
      }

// check_file("../keyboard");
// check_file("../keyboard1");
}
//-----------------------------------------------------------------------------
Unicode
Avec::unicode(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].unicode;
}
//-----------------------------------------------------------------------------
uint32_t
Avec::get_av_pos(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].av_pos;
}
//-----------------------------------------------------------------------------
Token
Avec::uni_to_token(Unicode uni, const char * loc)
{
CHT_Index idx = find_char(uni);
   if (idx != Invalid_CHT)   return Token(character_table[idx].token_tag, uni);

   // not found: try alternative characters.
   //
   idx = map_alternative_char(uni);
   if (idx != Invalid_CHT)   return Token(character_table[idx].token_tag,
                                          character_table[idx].unicode);

   CERR << endl << "Avec::uni_to_token() : Char " << UNI(uni) << " (" << uni
        << ") not found in ⎕AV! (called from " << loc << ")" << endl;

   return Token();
}
//-----------------------------------------------------------------------------
uint32_t Avec::find_av_pos(Unicode av)
{
const CHT_Index pos = find_char(av);

   if (pos < 0)   return MAX_AV;   // not found

   return character_table[pos].av_pos;
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::find_char(Unicode av)
{
int l = 0;
int h = sizeof(character_table)/sizeof(*character_table) - 1;

   for (;;)
       {
         if (l > h)   return Invalid_CHT;   // not in table.

         const int m((h + l)/2);
         const Unicode um = character_table[m].unicode;
         if      (av < um)   h = m - 1;
         else if (av > um)   l = m + 1;
         else                return CHT_Index(m);
       }
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::map_alternative_char(Unicode alt_av)
{
   // map characters that look similar to characters used in GNU APL
   // to the GNU APL character.
   //
   switch(alt_av)
      {
        case 0x005E: return AV_AND;              //  map ^ to ∧
        case 0x007C: return AV_DIVIDES;          //  map | to ∣
        case 0x007E: return AV_TILDE_OPERATOR;   //  map ~ to ∼
        case 0x03B1: return AV_ALPHA;            //  map α to ⍺
        case 0x03B5: return AV_ELEMENT;          //  map ε to ∈
        case 0x03B9: return AV_IOTA;             //  map ι to ⍳
        case 0x03C1: return AV_RHO;              //  map ρ to ⍴
        case 0x03C9: return AV_OMEGA;            //  map ω to ⍵
        case 0x2018: return AV_SINGLE_QUOTE;     //  map ‘ to '
        case 0x2019: return AV_SINGLE_QUOTE;     //  map ’ to '
        case 0x220A: return AV_ELEMENT;          //  map ∊ to ∈
        case 0x2212: return AV_ASCII_MINUS;      //  map − to -
        case 0x22BC: return AV_NAND;             //  map ⊽ to ⍲
        case 0x22BD: return AV_NOR;              //  map ⊼ to ⍱
        case 0x22C4: return AV_DIAMOND;          //  map ⋄ to ◊
        case 0x25AF: return AV_QUAD_QUAD;        //  map ▯ to ⎕
        case 0x25E6: return AV_RING_OPERATOR;    //  map ◦ to ∘
        case 0x26AA: return AV_CIRCLE;           //  map ⚪ to ○
        case 0x2A7D: return AV_LESS_OR_EQUAL;    //  map ⩽ to ≤
        case 0x2A7E: return AV_MORE_OR_EQUAL;    //  map ⩾ to ≥
        case 0x2B25: return AV_DIAMOND;          //  map ⬥ to ◊
        case 0x2B26: return AV_DIAMOND;          //  map ⬦ to ◊
        case 0x2B27: return AV_DIAMOND;          //  map ⬧ to ◊
      }

   return Invalid_CHT;
}
//-----------------------------------------------------------------------------
bool
Avec::is_known_char(Unicode av)
{
   switch(av)
      {
#define char_def(_n, u, _t, _f, _p)  case u: 
#define char_df1(_n, u, _t, _f, _p)  case u: 
#include "Avec.def"
          return true;
      }

   return false;   // not found
}
//-----------------------------------------------------------------------------
bool
Avec::is_symbol_char(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_SYMBOL) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::is_first_symbol_char(Unicode av)
{
   return is_symbol_char(av) && ! is_digit(av);
}
//-----------------------------------------------------------------------------
bool
Avec::is_digit(Unicode av)
{
   return (av <= UNI_ASCII_9 && av >= UNI_ASCII_0);
}
//-----------------------------------------------------------------------------
bool
Avec::is_digit_or_space(Unicode av)
{
   return (av <= UNI_ASCII_SPACE || av <= UNI_ASCII_9 && av >= UNI_ASCII_0);
}
//-----------------------------------------------------------------------------
bool
Avec::is_number(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NUMERIC) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_after(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_AFTER) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_before(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_BEFORE) != 0;
}
//-----------------------------------------------------------------------------
Unicode
Avec::subscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2080);
        case 1: return Unicode(0x2081);
        case 2: return Unicode(0x2082);
        case 3: return Unicode(0x2083);
        case 4: return Unicode(0x2084);
        case 5: return Unicode(0x2085);
        case 6: return Unicode(0x2086);
        case 7: return Unicode(0x2087);
        case 8: return Unicode(0x2088);
        case 9: return Unicode(0x2089);
      }

   return Unicode(0x2093);
}
//-----------------------------------------------------------------------------
Unicode
Avec::superscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2070);
        case 1: return Unicode(0x00B9);
        case 2: return Unicode(0x00B2);
        case 3: return Unicode(0x00B3);
        case 4: return Unicode(0x2074);
        case 5: return Unicode(0x2075);
        case 6: return Unicode(0x2076);
        case 7: return Unicode(0x2077);
        case 8: return Unicode(0x2078);
        case 9: return Unicode(0x2079);
      }

   return Unicode(0x207A);
}
//-----------------------------------------------------------------------------
