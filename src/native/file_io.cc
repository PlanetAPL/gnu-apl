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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../Native_interface.hh"

   // CONVENTION: all functions must have an axis argument (like X
   // in A fun[X] B); the axis argument is a function number that selects
   // one of several functions provided by this library...
   //

/// the default buffer size if the user does not provide one
enum { SMALL_BUF = 5000 };

//-----------------------------------------------------------------------------
struct file_entry
{
   file_entry(FILE * fp, int fd)
   : fe_file(fp),
     fe_fd(fd),
     fe_errno(0),
     fe_may_read(false),
     fe_may_write(false)
   {}
     
   FILE * fe_file;     ///< FILE * returned by fopen
   int    fe_fd;       ///< file desriptor == fileno(file)
   int    fe_errno;    ///< errno for last operation
   bool   fe_may_read;    ///< file open for reading
   bool   fe_may_write;   ///< file open for writing
};

/// all files currently open
vector<file_entry> open_files;

static file_entry &
get_file(const Value & value)
{
   if (open_files.size() == 0)   // first access: init stdin, stdout, and stderr
      {
        file_entry f0(stdin, STDIN_FILENO);
        open_files.push_back(f0);
        file_entry f1(stdout, STDOUT_FILENO);
        open_files.push_back(f1);
        file_entry f2(stderr, STDERR_FILENO);
        open_files.push_back(f2);
      }

const APL_Float qct = Workspace::get_CT();
const APL_Integer handle = value.get_ravel(0).get_near_int(qct);


   loop(h, open_files.size())
      {
        if (open_files[h].fe_fd == handle)   return open_files[h];
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
static Token
do_printf(FILE * out, Value_P A)
{
   // A is expected to be a nested APL value. The first element shall be a 
   // format string, followed by the values for each % field. Result is the
   // number of characters (not bytes!) printed.
   //
int out_len = 0;
int a = 0;   // index into A
Value_P format = A->get_ravel(a++).get_pointer_value();

   for (int f = 0; f < format->element_count(); /* no f++ */ )
       {
         const Unicode uni = format->get_ravel(f++).get_char_value();
         if (uni != UNI_ASCII_PERCENT)   // not %
            {
              UCS_string ucs(uni);
              UTF8_string utf(ucs);
              fwrite(utf.c_str(), 1, utf.size(), out);
              out_len++;
              continue;
            }

         // % seen. copy field to fmt and fprintf it with the next argument
         //
         char fmt[40];
         int fm = 0;   // an index into fmt;
         fmt[fm++] = '%';
         for (; fm < (sizeof(fmt) - 1); ++fm)
             {
               if (f >= format->element_count())
                  {
                    // end of format reached without seeing the format char.
                    // we print the string and are done.
                    //
                    fwrite(fmt, 1, fm, out);
                    out_len += fm;
                    break;
                  }
               else
                  {
                    const Unicode un1 = format->get_ravel(f++).get_char_value();
                    switch(un1)
                       {
                         // flag chars and field width/precision
                         //
                         case '#':
                         case '0' ... '9':
                         case '-':
                         case ' ':
                         case '+':
                         case '\'':   // SUSE
                         case 'I':    // glibc
                         case '.':

                         // length modifiers
                         //
                         case 'h':
                         case 'l':
                         case 'L':
                         case 'q':
                         case 'j':
                         case 'z':
                         case 't': fmt[fm++] = un1;
                                   continue;

                         // conversion specifier
                         //
                         case 'd':   case 'i':   case 'o':
                         case 'u':   case 'x':   case 'X':   case 'p':
                              {
                                const APL_Integer iv =
                                            A->get_ravel(a++).get_int_value();
                                fmt[fm++] = un1;
                                fmt[fm] = 0;
                                out_len += fprintf(out, fmt, iv);
                              }
                              goto field_done;

                         case 'e':   case 'E':   case 'f':   case 'F':
                         case 'g':   case 'G':   case 'a':   case 'A':
                              {
                                const APL_Float fv =
                                            A->get_ravel(a++).get_real_value();
                                fmt[fm++] = un1;
                                fmt[fm] = 0;
                                out_len += fprintf(out, fmt, fv);
                              }
                              goto field_done;

                         case 's':   // string or char
                              if (A->get_ravel(a).is_character_cell())
                                 goto cval;
                              {
                                Value_P str =
                                        A->get_ravel(a++).get_pointer_value();
                                UCS_string ucs(*str.get());
                                UTF8_string utf(ucs);
                                fwrite(utf.c_str(), 1, utf.size(), out); 
                                out_len += ucs.size();   // not utf.size() !
                              }
                              goto field_done;

                         case 'c':   // single char
                         cval:
                              {
                                const Unicode cv =
                                            A->get_ravel(a++).get_char_value();
                                UCS_string ucs(cv);
                                UTF8_string utf(ucs);
                                fwrite(utf.c_str(), 1, utf.size(), out); 
                                ++out_len;
                              }
                              goto field_done;

                         case 'n':
                              out_len += fprintf(out, "%d", out_len);
                              goto field_done;

                         case 'm':
                              out_len += fprintf(out, "%s", strerror(errno));
                              goto field_done;


                         case '%':
                              if (fm == 0)   // %% is %
                                 {
                                   fputc('%', out);
                                   ++out_len;
                                   goto field_done;
                                 }
                              /* no break */

                         default:
                             {
                               UCS_string & t4 = Workspace::more_error();
                               t4.clear();
                               t4.append_utf8("invalid format character ");
                               t4.append(un1, LOC);
                               t4.append_utf8(" in function 22 (aka. printf())"
                                              " in module file_io:: ");
                               DOMAIN_ERROR;   // bad format char
                             }
                       }
                  }
             }
         field_done: ;
       }

   

Value_P Z(new Value(LOC));   // skalar result
   new (Z->next_ravel())   IntCell(out_len);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
extern "C" void * get_function_mux(const char * function_name);
static Fun_signature get_signature();
static Token eval_B(Value_P B);
static Token eval_AB(Value_P A, Value_P B);
static Token eval_XB(Value_P X, Value_P B);
static Token eval_AXB(Value_P A, Value_P X, Value_P B);

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "eval_B"))          return (void *)&eval_B;
   if (!strcmp(function_name, "eval_AB"))         return (void *)&eval_AB;
   if (!strcmp(function_name, "eval_XB"))         return (void *)&eval_XB;
   if (!strcmp(function_name, "eval_AXB"))        return (void *)&eval_AXB;
   return 0;
}
//-----------------------------------------------------------------------------
/// a mandatory function that returns the signature of the eval_XXX()
/// function(s) provided by this library.
///
Fun_signature
get_signature()
{
   return SIG_Z_A_F2_B;
}
//-----------------------------------------------------------------------------
static Token
list_functions(ostream & out)
{
   out << 
"   Functions provided by this library.\n"
"   Assumes 'lib_file_io.so'  ⎕FX  'FUN'\n"
"\n"
"   Legend: e - error code\n"
"           i - integer\n"
"           h - file handle (integer)\n"
"           s - string\n"
"           A1, A2, ...  nested vector with elements A1, A2, ...\n"
"\n"
"           FUN     ''    print this text on stderr\n"
"        '' FUN     ''    print this text on stdout\n"
"           FUN[ 0] ''    print this text on stderr\n"
"        '' FUN[ 0] ''    print this text on stdout\n"
"\n"
"   Zi ←    FUN[ 1] ''    errno (of last call)\n"
"   Zs ←    FUN[ 2] Be    strerror(Be)\n"
"   Zh ← As FUN[ 3] Bs    fopen(Bs, As) filename Bs mode As\n"
"   Zh ←    FUN[ 3] Bs    fopen(Bs, \"r\") filename Bs\n"
"\n"
"   Ze ←    FUN[ 4] Bh    fclose(Bh)\n"
"   Ze ←    FUN[ 5] Bh    errno (of last call on Bh)\n"
"   Zi ←    FUN[ 6] Bh    fread(Zi, 1, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 6] Bh    fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 7] Bh    fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[ 8] Bh    fgets(Zi, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 8] Bh    fgets(Zi, Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[ 9] Bh    fgetc(Zi, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[10] Bh    feof(Bh)\n"
"   Zi ←    FUN[11] Bh    ferror(Bh)\n"
"   Zi ←    FUN[12] Bh    ftell(Bh)\n"
"   Zi ← Ai FUN[13] Bh    fseek(Bh, Ai, SEEK_SET)\n"
"   Zi ← Ai FUN[14] Bh    fseek(Bh, Ai, SEEK_CUR)\n"
"   Zi ← Ai FUN[15] Bh    fseek(Bh, Ai, SEEK_END)\n"
"   Zi ←    FUN[16] Bh    fflush(Bh)\n"
"   Zi ←    FUN[17] Bh    fsync(Bh)\n"
"   Zi ←    FUN[18] Bh    fstat(Bh)\n"
"   Zi ←    FUN[19] Bh    unlink(Bc)\n"
"   Zi ←    FUN[20] Bh    mkdir(Bc, 0777)\n"
"   Zi ← Ai FUN[20] Bh    mkdir(Bc, AI)\n"
"   Zi ←    FUN[21] Bh    rmdir(Bc)\n"
"   Zi ← A  FUN[22] 1     printf(         A1, A2...) format A1\n"
"   Zi ← A  FUN[22] 2     fprintf(stderr, A1, A2...) format A1\n"
"   Zi ← A  FUN[22] Bh    fprintf(Bh,     A1, A2...) format A1\n"
"\n";

   return Token(TOK_APL_VALUE1, Value::Str0_P);
}
//-----------------------------------------------------------------------------
Token
eval_B(Value_P B)
{
   return list_functions(CERR);
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B)
{
   return list_functions(COUT);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(CERR);

         case 1:   // return (last) errno
              goto out_errno;

         case 2:   // return strerror(B)
              {
                const int b = B->get_ravel(0).get_near_int(qct);
                const char * text = strerror(b);
                const int len = strlen(text);
                Value_P Z(new Value(len, LOC));
                loop(t, len)
                    new (&Z->get_ravel(t))   CharCell((Unicode)(text[t]));

                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 3:   // fopen(Bs, \"r\") filename Bs
              {
                UTF8_string path(*B.get());
                errno = 0;
                FILE * f = fopen(path.c_str(), "r");
                if (f == 0)   return Token(TOK_APL_VALUE1, Value::Minus_One_P);

                file_entry fe(f, fileno(f));
                fe.fe_may_read = true;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 4:   // fclose(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                if (fe.fe_file)   fclose(fe.fe_file);
                else              close(fe.fe_fd);

                fe = open_files.back();       // move last file to fe
                open_files.pop_back();        // erase last file
                goto out_errno;
              }

         case 5:   // errno of Bh
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(fe.fe_errno);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 6:   // fread(Zi, 1, SMALL_BUF, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char buffer[SMALL_BUF];

                size_t len = fread(buffer, 1, SMALL_BUF, fe.fe_file);
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 8:   // fgets(Zi, SMALL_BUF, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char buffer[SMALL_BUF];

                const char * s = fgets(buffer, SMALL_BUF, fe.fe_file);
                const int len = s ? strlen(s) : 0;
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 9:   // fgetc(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(fgetc(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 10:   // feof(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(feof(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 11:   // ferror(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(ferror(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 12:   // ftell(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(ftell(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 16:   // fflush(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                fflush(fe.fe_file);
              }
              goto out_errno;

         case 17:   // fsync(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                fsync(fe.fe_fd);
              }
              goto out_errno;

         case 18:   // fstat(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                struct stat s;
                const int result = fstat(fe.fe_fd, &s);
                if (result)   goto out_errno;   // fstat failed

                Value_P Z(Value_P(new Value(13, LOC)));
                new (&Z->get_ravel( 0))   IntCell(s.st_dev);
                new (&Z->get_ravel( 1))   IntCell(s.st_ino);
                new (&Z->get_ravel( 2))   IntCell(s.st_mode);
                new (&Z->get_ravel( 3))   IntCell(s.st_nlink);
                new (&Z->get_ravel( 4))   IntCell(s.st_uid);
                new (&Z->get_ravel( 5))   IntCell(s.st_gid);
                new (&Z->get_ravel( 6))   IntCell(s.st_rdev);
                new (&Z->get_ravel( 7))   IntCell(s.st_size);
                new (&Z->get_ravel( 8))   IntCell(s.st_blksize);
                new (&Z->get_ravel( 9))   IntCell(s.st_blocks);
                new (&Z->get_ravel(10))   IntCell(s.st_atime);
                new (&Z->get_ravel(11))   IntCell(s.st_mtime);
                new (&Z->get_ravel(12))   IntCell(s.st_ctime);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 19:   // unlink(Bc)
              {
                UTF8_string path(*B.get());
                unlink(path.c_str());
              }
              goto out_errno;

         case 20:   // mkdir(Bc)
              {
                UTF8_string path(*B.get());
                mkdir(path.c_str(), 0777);
              }
              goto out_errno;

         case 21:   // rmdir(Bc)
              {
                UTF8_string path(*B.get());
                rmdir(path.c_str());
              }
              goto out_errno;
      }

   CERR << "Bad eval_XB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
Value_P Z(Value_P(new Value(LOC)));
   new (&Z->get_ravel(0))   IntCell(errno);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value_P A, const Value_P X, const Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(COUT);

         case 3:   // fopen(Bs, As) filename Bs mode As
              {
                UTF8_string path(*B.get());
                UTF8_string mode(*A.get());
                const char * m = mode.c_str();
                bool read = false;
                bool write = false;
                if      (!strncmp(m, "r+", 2))     read = write = true;
                else if (!strncmp(m, "r" , 1))     read         = true;
                else if (!strncmp(m, "w+", 2))     read = write = true;
                else if (!strncmp(m, "w" , 1))            write = true;
                else if (!strncmp(m, "a+", 2))     read = write = true;
                else if (!strncmp(m, "a" , 1))            write = true;
                else    DOMAIN_ERROR;

                errno = 0;
                FILE * f = fopen(path.c_str(), m);
                if (f == 0)   return Token(TOK_APL_VALUE1, Value::Minus_One_P);

                file_entry fe(f, fileno(f));
                fe.fe_may_read = read;
                fe.fe_may_write = write;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 6:   // fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int(qct);
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);


                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                size_t len = fread(buffer, 1, bytes, fe.fe_file);
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                delete del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 7:   // fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->nz_element_count();
                file_entry & fe = get_file(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                loop(z, bytes)   buffer[z] = A->get_ravel(z).get_near_int(qct);
                delete del;

                size_t len = fwrite(buffer, 1, bytes, fe.fe_file);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(len); 
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 8:   // fgets(Zi, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int(qct);
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(buffer))
                   buffer = del = new char[bytes + 1];

                const char * s = fgets(buffer, bytes, fe.fe_file);
                const int len = strlen(buffer);
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                delete del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 13:   // fseek(Bh, Ai, SEEK_SET)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_SET);
              }
              goto out_errno;

         case 14:   // fseek(Bh, Ai, SEEK_CUR)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_CUR);
              }
              goto out_errno;

         case 15:   // fseek(Bh, Ai, SEEK_END)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_END);
              }
              goto out_errno;

         case 20:   // mkdir(Bc, Ai)
              {
                const int mask = A->get_ravel(0).get_near_int(qct);
                UTF8_string path(*B.get());
                mkdir(path.c_str(), mask);
              }
              goto out_errno;

         case 22:   // fprintf(Bc, Ai)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                return do_printf(fe.fe_file, A);
              }
      }

   CERR << "eval_AXB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
Value_P Z(Value_P(new Value(LOC)));
   new (&Z->get_ravel(0))   IntCell(errno);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

