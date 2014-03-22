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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

#include "CDR_string.hh"
#include "APmain.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"

using namespace std;

enum { ALL_RW = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH };

struct SVAR_context
{
   SVAR_context(FILE * f, int c, Coupled_var & vC, Coupled_var & vD, bool wr)
   : file(f),
     var_C(vC),
     var_D(vD),
     write(wr),
     code(c),
     rec_num(0),
     rec_size(128),
     filepos(0),
     state_D(SVS_NOT_SHARED)
  {}

   ~SVAR_context()
      { assert(file == 0); }

   /// the (open) file to be read or written
   FILE * file;

   /// the (shared) CTL variable for the file
   Coupled_var & var_C;

   /// the (shared) DAT variable for the file
   Coupled_var & var_D;

   /// true if file is to be written
   const bool write;
   int code;

   /// the record size of the file
   int rec_num;

   /// the (next) record number in the file
   int rec_size;

   /// the file position of the file
   int filepos;

   /// state changes of DAT variable
   Svar_state state_D;
};

//-----------------------------------------------------------------------------
int
set_ACK(Coupled_var & var, int exco)
{
const uint8_t _cdr[] =
   {
     0x00, 0x00, 0x20, 0x20, // ptr
     0x00, 0x00, 0x00, 0x20, // nb: total len
     0x00, 0x00, 0x00, 0x01, // nelm: element count
     0x01, 0x00, 0x00, 0x00, // type, rank, pad, pad
     uint8_t(exco), uint8_t(exco>>8), uint8_t(exco>>16), uint8_t(exco>>24),
     0x00, 0x00, 0x00, 0x00, // pad
     0x00, 0x00, 0x00, 0x00, // pad
     0x00, 0x00, 0x00, 0x00, // pad
   };

const CDR_string * cdr = new CDR_string(_cdr, sizeof(_cdr));
   delete var.data;
   var.data = cdr;

   return exco;
}
//-----------------------------------------------------------------------------
int
write_fixed(FILE * file, int code, Coupled_var & var_D,
            int & filepos, int rn, int rs)
{
   if (code == 'A')        // APL2 format (CDR)
      {
      }
   else if (code == 'B')   // boolean fixed size
      {
      }
   else if (code == 'C')   // char fixed size
      {
      }
   else if (code == 'D')   // char (var size
      {
      }
   else if (code == 'T')   // translate
      {
      }
   else assert(0 && "Bad code at" LOC);

   return 0;
}
//-----------------------------------------------------------------------------
int
seek_variable(FILE * file, int code, int rn)
{
   rewind(file);

   // skip first rn records

   if (code == 'A')        // APL2 format (CDR)
      {
        loop(r, rn)
            {
              CDR_header header;
              const int hlen = fread(&header, 1, sizeof(CDR_header), file);
              if (hlen != sizeof(header))   return -44;   // delimiter not found

              const int nb = header.get_nb();
              const int rest = nb - sizeof(CDR_header);
              uint8_t buffer[nb];
              const int rlen = fread(buffer, 1, rest, file);
              if (rlen != rest)   return -44;   // delimiter not found
            }

        return 0;
      }
   else if (code == 'B')   // boolean fixed size
      {
      }
   else if (code == 'C')   // char fixed size
      {
      }
   else if (code == 'D')   // char (var size
      {
      }
   else if (code == 'T')   // translate
      {
      }
   else assert(0 && "Bad code at" LOC);

   return 0;
}
//-----------------------------------------------------------------------------
int
read_variable(FILE * file, int code, Coupled_var & var_D,
              int & filepos, int rn, int rs, bool skip_NL)
{
   if (filepos != rn)
      if (int result = seek_variable(file, code, rn))   return result;
   filepos = rn;

   if (code == 'A')        // APL2 format (CDR)
      {
        CDR_header header;
        const int hlen = fread(&header, 1, sizeof(header), file);
        if (hlen != sizeof(CDR_header))   return -44;   // delimiter not found

        const int nb = header.get_nb();
        const int rest = nb - sizeof(CDR_header);
        uint8_t buffer[nb];
        memcpy(buffer, &header, sizeof(CDR_header));
        const int rlen = fread(buffer + sizeof(CDR_header), 1, rest, file);
        if (rlen != rest)   return -44;   // delimiter not found

        delete var_D.data;
        var_D.data = new CDR_string(buffer, nb);

        return 0;
      }
   else if (code == 'D')   // char (var size
      {
      }
   else if (code == 'T')   // translate
      {
      }

   return -31;   // invalid code, i.e. var_C domain error
}
//-----------------------------------------------------------------------------
int
write_variable(FILE * file, int code, Coupled_var & var_D,
               int & filepos, int rn, int rs)
{
   // if rn is larger than filepos then append at the end
   //
   if (rn > filepos)   rn = filepos;

   if (rn < filepos)
      {
        const int result = seek_variable(file, code, rn);
        if (result)   return result;
      }
   filepos = rn;

   assert(var_D.data);
const CDR_string & cdr = *var_D.data;

   if (code == 'A')        // APL2 format (CDR)
      {
        const int len = cdr.header().get_nb();
        const uint8_t * data = cdr.get_items();
        const size_t written = fwrite(data, len, 1, file);
        if (written == len)   return -48;   // partial write

        ++filepos;
        return 0;
      }
   else if (code == 'D')   // char (var size
      {
      }
   else if (code == 'T')   // translate
      {
      }
   return -31;   // invalid code, i.e. var_C domain error
}
//-----------------------------------------------------------------------------
void
sub_command(SVAR_context & ctx)
{
int op = -1;
int result;

   assert(ctx.var_C.data);

const CDR_string & cdr = *ctx.var_C.data;

   if (cdr.check())
      {
        CERR << "Bad CDR record:" << endl;
        cdr.debug(CERR, LOC);
        set_ACK(ctx.var_C, -47);
        return;
      }

   if (cdr.get_rank() > 1)   // not a skalar or vector
      {
        CERR << "Bad CDR rank (" << cdr.get_rank() << endl;
        set_ACK(ctx.var_C, -32);   // C rank error
        return;
      }

const int nelm = cdr.header().get_nelm();
   if (nelm == 0)       // '' or ⍳0: close file
      {
        // close file
        fclose(ctx.file);
        ctx.file = 0;

        // return to command mode
        ctx.var_C.context = ctx.var_D.context = 0;
        delete &ctx;

        set_ACK(ctx.var_C, 0);
        return;
      }

   if (nelm  > 3)
      {
        set_ACK(ctx.var_C, -33);   // C length error
        return;
      }

   if (cdr.get_type() == CDR_BOOL1)
      {
        const uint8_t byte = *cdr.ravel();
        op                            = byte & 0x80 ? 1 : 0;
        if (nelm >= 2)   ctx.rec_num  = byte & 0x40 ? 1 : 0;
        if (nelm >= 3)   ctx.rec_size = byte & 0x20 ? 1 : 0;
      }
   else if (cdr.get_type() == CDR_INT32)
      {
        const uint32_t * ravel = (const uint32_t *)cdr.ravel();
        op                        = ravel[0];
        if (nelm >= 2)   ctx.rec_num  = ravel[1];
        if (nelm >= 3)   ctx.rec_size = ravel[2];
      }
   else   // neither int nor bool:
      {
        set_ACK(ctx.var_C, -31);    // C domain error 
        return;
      }

   switch(op)
      {
        case 0:  // read fixed-len record
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 return;
                 break;

        case 1:  // write fixed-len record
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 return;
                 break;

        case 2:  // read direct
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 return;
                 break;

        case 3:  // write direct
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 return;
                 break;

        case 4:  // read variable (APL object or line)
                 if (ctx.write)
                    {
                      CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                      set_ACK(ctx.var_C, -47);    // -47 INVALID SUBCOMMAND
                      return;
                    }

                 result = read_variable(ctx.file, ctx.code, ctx.var_D,
                                        ctx.filepos, ctx.rec_num,
                                        ctx.rec_size, false);
                 set_ACK(ctx.var_C, result);
                 ctx.state_D = SVS_OFF_HAS_SET;
                 if (result)   return;
                 ctx.rec_num = ctx.filepos;
                 break;

        case 5:  // write variable (APL object or line)
                 if (!ctx.write)
                    {
                      CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                      set_ACK(ctx.var_C, -47);    // -47 INVALID SUBCOMMAND
                      return;
                    }

                 result = write_variable(ctx.file, ctx.code, ctx.var_D,
                                        ctx.filepos, ctx.rec_num, ctx.rec_size);
                 set_ACK(ctx.var_C, result);
                 ctx.state_D = SVS_IDLE;
                 if (result)   return;
                 ctx.rec_num = ctx.filepos;
                 break;

        case 6:  // read variable (APL object or line) without CR/LF
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 return;
                       break;

        default: set_ACK(ctx.var_C, -47);    // -47 := INVALID SUBCOMMAND
                 CERR << " INVALID SUBCOMMAND at " << LOC << endl;
                 return;
      }
}
//-----------------------------------------------------------------------------
FILE *
open_pipe(const char * filename, const char * mode)
{
   // if filename does not exist, create a named pipe with that name
   //
struct stat st;
   if (stat(filename, &st) == 0)   // file exists
      {
        if (S_ISFIFO(st.st_mode))   return fopen(filename, mode);
        return 0;   // file exists but is not a pipe
      }

   // stat failed: create pipe
   //
   if (mknod(filename, S_IFIFO | ALL_RW, 0) == 0)   // mknod successful
      return fopen(filename, mode);

   return 0;
}
//-----------------------------------------------------------------------------
void
read_pipe(const char * filename, int code,
          Coupled_var & var_C, Coupled_var & var_D)
{
   if (code == 0)   code = 'A';    // default if no code is provided
   if (!strchr("ABCDT", code))
      {
       set_ACK(var_C, -31);        // -31: CTL DOMAIN ERROR
       return;
      }

FILE * f = open_pipe(filename, "r");
   if (f == 0)
      {
       set_ACK(var_C, 2);          //  2 := FILE NOT FOUND
       return;
      }

SVAR_context * ctx = new SVAR_context(f, code, var_C, var_D, false);
   var_C.context = var_D.context = ctx;
   set_ACK(var_C, 0);
   set_ACK(var_D, 0);
}
//-----------------------------------------------------------------------------
void
write_pipe(const char * filename, int code, Coupled_var & var_C, Coupled_var & var_D)
{
   if (code == 0)   code = 'A';    // default if no code is provided
   if (!strchr("ABCDT", code))
      {
       set_ACK(var_C, -31);        // -31: CTL DOMAIN ERROR
       return;
      }

FILE * f = open_pipe(filename, "a+");
   if (f == 0)
      {
       set_ACK(var_C, 2);          // 2 := FILE NOT FOUND
       return;
      }

SVAR_context * ctx = new SVAR_context(f, code, var_C, var_D, true);
   var_C.context = var_D.context = ctx;
   set_ACK(var_C, 0);
   set_ACK(var_D, 0);
}
//-----------------------------------------------------------------------------
void
read_file(const char * filename, int code, Coupled_var & var_C, Coupled_var & var_D)
{
   if (code == 0)   code = 'A';    // default if no code is provided
   if (!strchr("ABCDT", code))
      {
       set_ACK(var_C, -31);        // -31: CTL DOMAIN ERROR
       return;
      }

FILE * f = fopen(filename, "r");
   if (f == 0)
      {
       set_ACK(var_C, 2);          //  2 := FILE NOT FOUND
       return;
      }

   fseek(f, 0, SEEK_END);
   set_ACK(var_D, ftell(f));

SVAR_context * ctx = new SVAR_context(f, code, var_C, var_D, false);
   var_C.context = var_D.context = ctx;
   fseek(f, 0, SEEK_SET);
   set_ACK(var_C, 0);
}
//-----------------------------------------------------------------------------
void
write_file(const char * filename, int code, Coupled_var & var_C, Coupled_var & var_D)
{
   if (code == 0)   code = 'A';    // default if no code is provided
   if (!strchr("ABCDT", code))
      {
        set_ACK(var_C, -31);        // -31: CTL DOMAIN ERROR
        return;
      }

FILE * f = fopen(filename, "w");
   if (f == 0)
      {
       set_ACK(var_C, 2);          //  2 := FILE NOT FOUND
       return;
      }

   fseek(f, 0, SEEK_END);
SVAR_context * ctx = new SVAR_context(f, code, var_C, var_D, true);
   var_C.context = var_D.context = ctx;
   set_ACK(var_C, 0);
   set_ACK(var_D, ftell(f));
}
//-----------------------------------------------------------------------------
void
delete_file(const char * filename, Coupled_var & var_C)
{
const int ret = unlink(filename) ? errno : 0;
   set_ACK(var_C, ret);
}
//-----------------------------------------------------------------------------
void
rename_file(const char * from, const char * to, Coupled_var & var_C)
{
const int ret = rename(from, to) ? errno : 0;
   set_ACK(var_C, ret);
}
//-----------------------------------------------------------------------------
bool
parse_arg(const char * & cmd, char * arg, int arglen, int minlen)
{
int arg_chars = 0;
bool error = false;
bool quoted = false;

   while (!error)
      {
         if (*cmd == 0)              // end of cmd string reached
            {
              error = quoted;   // error iff quote is missing
              break;
            }

         const char cc = *cmd++;
         if (cc == '"')              // start or end of quoted arg
            {
              quoted = !quoted;
              continue;
            }

         if (cc == ',' && !quoted)   // end of arg reached
            break;

         // argument character: copy it.
         //
         if (arg_chars < arglen)   arg[arg_chars++] = cc;
         else                      error = true;
      }

   arg[arg_chars] = 0;
   if (arg_chars < minlen)   error = true;
   return error;
}
//-----------------------------------------------------------------------------
void
handle_cmd(const char * cmd, int len, Coupled_var & var_C, Coupled_var & var_D)
{
   // cmd is a string containing 1, 2, or 3 arguments separated by commas.
   // parse the arguments.
   //
char arg1[3];
char arg2[FILENAME_MAX+2];
char arg3[FILENAME_MAX+2];

   if (parse_arg(cmd, arg1, sizeof(arg1) - 1, 2))   set_ACK(var_C, 401);
   if (parse_arg(cmd, arg2, sizeof(arg2) - 1, 1))   set_ACK(var_C, 402);
   if (parse_arg(cmd, arg3, sizeof(arg3) - 1, 0))   set_ACK(var_C, 403);

const char c0 = arg1[0];
const char c1 = arg1[1];
   if      (c0 == 'I' && c1 == 'R')   read_file  (arg2, *arg3, var_C, var_D);
   else if (c0 == 'I' && c1 == 'W')   write_file (arg2, *arg3, var_C, var_D);
   else if (c0 == 'P' && c1 == 'R')   read_pipe  (arg2, *arg3, var_C, var_D);
   else if (c0 == 'P' && c1 == 'W')   write_pipe (arg2, *arg3, var_C, var_D);
   else if (c0 == 'D' && c1 == 'L')   delete_file(arg2,        var_C);
   else if (c0 == 'R' && c1 == 'N')   rename_file(arg2,  arg3, var_C);
   else    set_ACK(var_C, 1);   // 1 := INVALID COMMAND
}
//-----------------------------------------------------------------------------
bool
is_valid_varname(const uint32_t * varname)
{
   // AP210 supports only control variables starting with 'C' and data
   // variables starting with 'D'.
   //
   if (varname[0] == 'C')   return true;
   if (varname[0] == 'D')   return true;
   return false;
}
//-----------------------------------------------------------------------------
/// return true iff ctl/dat is CTL/DAT, or Cxxx/Dxxx (or vice versa)
static bool
compare_other(const uint32_t * ctl, const uint32_t * dat)
{
   if (*ctl == 'D' && *dat == 'C')   // maybe vice versa: change roles
      return compare_other(dat, ctl);

   if (*ctl++ != 'C')   return false;   // not CTL or Cxxx
   if (*dat++ != 'D')   return false;   // not DAT or Dxxx

   // check CTL vs. DAT
   //
   if (ctl[0] == 'T' && ctl[1] == 'L' && ctl[2] == 0)
      {
        if (dat[0] != 'A')   return false;
        if (dat[1] != 'T')   return false;
        if (dat[2] == 0)     return true;

        // continue since CTL might match DTL as well
      }

   for (; ; ++ctl, ++dat)
       {
         if (*ctl != *dat)   return false;
         if (*ctl == 0)      return true;
       }

   /* not reached */
   return false;
}
//-----------------------------------------------------------------------------
bool
initialize(Coupled_var & var)
{
   coupled_vars.push_back(var);
const offered_SVAR * svar =  Svar_DB::find_var(var.key);
   if (!svar)   return true;

   if (*svar->varname == 'C')
      {
        Svar_DB::set_control(svar->key, USE_BY_1);
      }
   else if (*svar->varname == 'D')
      {
      }
   else                             return true;  // error
   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
make_counter_offer(SV_key key)
{
   return true;   // make counter offer
}
//-----------------------------------------------------------------------------
APL_error_code
assign_value(Coupled_var & var, const string & data)
{
   // check CDR size, type, and shape of data. We expect a char string
   // for both the CTL and the DAT variable.
   //
   if (data.size() < sizeof(CDR_header))   // too short
      { error_loc = LOC;   return E_LENGTH_ERROR; }

const CDR_header * header = (const CDR_header *)data.c_str();

const int rank = header->rank;

offered_SVAR * svar =  Svar_DB::find_var(var.key);
   if (svar == 0)   { error_loc = LOC;   return E_VALUE_ERROR; }

   if (*svar->varname == 'D')   // e.g. DAT←value
      {
        delete var.data;
        var.data = new CDR_string((const uint8_t *)data.c_str(), data.size());
        return E_NO_ERROR;
      }

   // CTL←value
   //
   if (rank > 1)
      { error_loc = LOC;   return E_RANK_ERROR; }

SV_key key_D = Svar_DB::pairing_key(var.key, compare_other);
   if (key_D == 0)   { error_loc = LOC;   return E_VALUE_ERROR; }
Coupled_var * var_D = 0;
   for (int c = 0; c < coupled_vars.size(); ++c)
       {
         if (coupled_vars[c].key == key_D)
            {
              var_D = &coupled_vars[c];
              break;
            }
       }
   if (var_D == 0)
      {
        CERR << "key_D = " << (key_D & 0xFFFF) << endl;
        for (int c = 0; c < coupled_vars.size(); ++c)
            CERR << "key = " << (coupled_vars[c].key & 0xFFFF) << endl;

        error_loc = LOC;   return E_VALUE_ERROR;
      }


   delete var.data;
   var.data = new CDR_string((const uint8_t *)data.c_str(), data.size());

SVAR_context * ctx = var.context;
   if (ctx)
      {
        ctx->state_D = SVS_NOT_SHARED;
        sub_command(*ctx);

        // a sub-command was triggered by a write to C, so C is always written.
        // in addition D may have been read  or written.
        //
        svar->set_state(false, LOC);

        if (ctx->state_D != SVS_NOT_SHARED)   // var D read or written
           {
             offered_SVAR * svar_D =  Svar_DB::find_var(key_D);
             if (svar_D == 0)   { error_loc = LOC;   return E_VALUE_ERROR; }
             svar_D->set_state(ctx->state_D == SVS_IDLE, LOC);
           }
      }
   else
      {
        const char * cmd = (const char *)(header + 1) + 4*rank;
        handle_cmd(cmd, data.size(), var, *var_D);
offered_SVAR * svar_D =  Svar_DB::find_var(key_D);
   if (svar_D == 0)   { error_loc = LOC;   return E_VALUE_ERROR; }
   svar->set_state(false, LOC);
   svar_D->set_state(false, LOC);
      }

   error_loc = "no_error";   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
APL_error_code
get_value(Coupled_var & var, string & data)
{
   if (var.data == 0)   return E_VALUE_ERROR;

   data = string((const char *)(var.data->get_items()), var.data->size());
   error_loc = "no_error";   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
void
retract(Coupled_var & var)
{
SVAR_context * ctx = var.context;
   if (ctx)
      {
        if (ctx->file)   fclose(ctx->file);
        ctx->file = 0;

        ctx->var_C.context = ctx->var_D.context = 0;
        delete ctx;
      }
}
//-----------------------------------------------------------------------------
