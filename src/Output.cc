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

#include "../config.h"   // for HAVE_ macros from configure

#if HAVE_CURSES_H && HAVE_CURSES_H

#include <curses.h>
#include <term.h>

#else

#define tputs(x, y, z)
#define setupterm(x, y, z) -1

#endif

#include "Command.hh"
#include "Common.hh"
#include "DiffOut.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Svar_DB.hh"
#include "TestFiles.hh"

bool Output::colors_enabled = false;
bool Output::colors_changed = false;
bool Output::print_sema_held = false;
bool Output::use_curses = true;

int Output::color_CIN_foreground = 0;
int Output::color_CIN_background = 7;
int Output::color_COUT_foreground = 0;
int Output::color_COUT_background = 8;
int Output::color_CERR_foreground = 5;
int Output::color_CERR_background = 8;
int Output::color_UERR_foreground = 5;
int Output::color_UERR_background = 8;

/// a filebuf for stdin echo
class CinOut : public filebuf
{
   /// overloaded filebuf::overflow
   virtual int overflow(int c);
} cin_filebuf;

/// a filebuf for stderr output
class ErrOut : public filebuf
{
protected:
   /// overloaded filebuf::overflow()
   virtual int overflow(int c);

public:
   /** a helper function telling whether the constructor for CERR was called
       if CERR is used before its constructor was called (which can happen in
       when constructors of static objects are called and use CERR) then a
       segmentation fault would occur.

       We avoid that by using get_CERR() instead of CERR in such constructors.
       get_CERR() checks \b used and returns cerr instead of CERR if it is
       false.
    **/
   filebuf * use()   { used = true;   return this; }

   /// true iff the constructor for CERR was called
   static bool used;   // set when CERR is constructed

} cerr_filebuf;   ///< a filebuf for CERR

bool ErrOut::used = false;

DiffOut dout_filebuf(false);
DiffOut uerr_filebuf(true);

ostream CIN(&cin_filebuf);
ostream COUT(&dout_filebuf);
ostream CERR(cerr_filebuf.use());
ostream UERR(&uerr_filebuf);

extern ostream & get_CERR();
ostream & get_CERR()
{
   return ErrOut::used ? CERR : cerr;
};

Output::ColorMode Output::color_mode = COLM_UNDEF;

/// VT100 escape sequence to change to cin color
char Output::color_CIN[21] =
 { 27, 91, '0', ';', '3', '0', ';', '4', '7', 'm', 0 };

/// VT100 escape sequence to change to cout color
char Output::color_COUT[21] =
   { 27, 91, '0', ';', '3', '0', ';', '4', '8', 'm', 0 };

/// VT100 escape sequence to change to cerr color
char Output::color_CERR[21] =
   { 27, 91, '0', ';', '3', '5', ';', '4', '8', 'm', 0 };

/// VT100 escape sequence to change to cerr color
char Output::color_UERR[21] =
   { 27, 91, '0', ';', '3', '5', ';', '4', '8', 'm', 0 };

/// VT100 escape sequence to reset colors to their default
char Output::color_RESET[21] =
  { 27, 91, '0', ';', '3', '8', ';', '4', '8', 'm', 0 };

/// VT100 escape sequence to clear to end of line
char Output::clear_EOL[21] =
   { 27, 91, 'K', 0 };

//-----------------------------------------------------------------------------
int
CinOut::overflow(int c)
{
   if (do_not_echo)   return 0;
   if (!Output::print_sema_held)
      {
        Svar_DB::start_print(LOC);
        Output::print_sema_held = true;
      }

   Output::set_color_mode(Output::COLM_INPUT);
   cerr << (char)c;

   if (c == 0x0A)
      {
        if (Output::print_sema_held)
           {
             Svar_DB::end_print(LOC);
             Output::print_sema_held = false;
           }
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
ErrOut::overflow(int c)
{
   if (!Output::print_sema_held)
      {
        Svar_DB::start_print(LOC);
        Output::print_sema_held = true;
      }

   Output::set_color_mode(Output::COLM_ERROR);
   cerr << (char)c;

   if (c == '\n')
      {
        Svar_DB::end_print(LOC);
        Output::print_sema_held = false;
      }

   return 0;
}
//-----------------------------------------------------------------------------
void
Output::init()
{
int ret = setupterm(NULL, fileno(stdout), NULL);
   if (ret != 0)
      {
        use_curses = false;
      }
}
//-----------------------------------------------------------------------------
int
Output::putc_stderr(int ch)
{
   cerr << (char)ch;
   return ch;
}
//-----------------------------------------------------------------------------
int
Output::putc_stdout(int ch)
{
   cout << (char)ch;
   return ch;
}
//-----------------------------------------------------------------------------
void
Output::reset_dout()
{
   dout_filebuf.reset();
}
//-----------------------------------------------------------------------------
void
Output::reset_colors()
{
   if (!colors_changed)   return;

   if (use_curses)
      {
        tputs(exit_attribute_mode, 1, putc_stdout);
        tputs(clr_eol, 1, putc_stdout);

        tputs(exit_attribute_mode, 1, putc_stderr);
        tputs(clr_eol, 1, putc_stderr);
      }
   else
      {
        cout << color_RESET << clear_EOL;
        cerr << color_RESET << clear_EOL;
      }
}
//-----------------------------------------------------------------------------
void
Output::set_color_mode(Output::ColorMode mode)
{
   if (!colors_enabled)      return;   // colors disabled
   if (color_mode == mode)   return;   // no change in color mode

   // mode changed
   //
   color_mode = mode;
   colors_changed = true;   // to reset them in reset_colors()

   if (use_curses)
      {
        switch(color_mode)
           {
             case COLM_INPUT:
                  tputs(tparm(set_foreground, color_CIN_foreground),
                              1, putc_stdout);
                  tputs(tparm(set_background, color_CIN_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

             case COLM_OUTPUT:
                  tputs(tparm(set_foreground, color_COUT_foreground),
                              1, putc_stdout);
                  tputs(tparm(set_background, color_COUT_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

             case COLM_ERROR:
                  tputs(tparm(set_foreground, color_CERR_foreground),
                              1, putc_stderr);
                  tputs(tparm(set_background, color_CERR_background),
                              1, putc_stderr);
                  tputs(clr_eol, 1, putc_stderr);
                  break;

             case COLM_UERROR:
                  tputs(tparm(set_foreground, color_UERR_foreground),
                              1, putc_stdout);
                  tputs(tparm(set_background, color_UERR_background),
                              1, putc_stdout);
                  tputs(clr_eol, 1, putc_stdout);
                  break;

           }
      }
   else
      {
        switch(color_mode)
           {
             case COLM_INPUT:  cerr << color_CIN  << clear_EOL;   break;
             case COLM_OUTPUT: cout << color_COUT << clear_EOL;   break;
             case COLM_ERROR:  cerr << color_CERR << clear_EOL;   break;
             case COLM_UERROR: cout << color_UERR << clear_EOL;   break;
           }
      }
}
//-----------------------------------------------------------------------------
void 
Output::toggle_color(const UCS_string & arg)
{
int a = 0;
   while (a < arg.size() && arg[a] < UNI_ASCII_SPACE)   ++a;

   if (arg.starts_iwith("ON"))         colors_enabled = true;
   else if (arg.starts_iwith("OFF"))   colors_enabled = false;
   else                                colors_enabled = !colors_enabled;
}
//-----------------------------------------------------------------------------
