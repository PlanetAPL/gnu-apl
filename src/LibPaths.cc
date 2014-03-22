/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2013  Dr. Jürgen Sauermann

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Error.hh"
#include "LibPaths.hh"
#include "PrintOperator.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

bool LibPaths::root_from_env = false;
bool LibPaths::root_from_pwd = false;

char LibPaths::APL_bin_path[PATH_MAX + 1] = "";
char LibPaths::APL_lib_root[PATH_MAX + 10] = "";

LibPaths::LibDir LibPaths::lib_dirs[LIB_MAX];

//-----------------------------------------------------------------------------
void LibPaths::init(const char * argv0)
{
   compute_bin_path(argv0);
   search_APL_lib_root();

   loop(d, LIB_MAX)
      {
        if (root_from_env)        lib_dirs[d].cfg_src = LibDir::CS_ENV;
        else if (root_from_pwd)   lib_dirs[d].cfg_src = LibDir::CS_ARGV0;
        else                      lib_dirs[d].cfg_src = LibDir::CS_NONE;
      }
}
//-----------------------------------------------------------------------------
void LibPaths::compute_bin_path(const char * argv0)
{
   // compute APL_bin_path from argv0
   //
   if (strchr(argv0, '/') == 0)
      {
         // if argv0 contains no / then realpath() seems to prepend the current
         // directory to argv0 (which is wrong since argv0 may be in $PATH).
         //
         // we fix this by searching argv0 in $PATH
         //
         const char * path = getenv("PATH");   // must NOT be modified

          if (path)
            {
              const size_t alen = strlen(argv0);
              const size_t plen = strlen(path);
              char path1[plen + 1];
              strncpy(path1, path, sizeof(path1));
              char * next = path1;
              for (;;)
                  {
                    char * semi = strchr(next, ':');
                    if (semi)   *semi = 0;
                    char filename[plen + alen + 10];
                    snprintf(filename, sizeof(filename), "%s/%s",
                             next, argv0);

                    if (access(filename, X_OK) == 0)
                       {
                         strncpy(APL_bin_path, next, sizeof(APL_bin_path));
                         return;
                       }

                    if (semi == 0)   break;
                    next = semi + 1;
                  }
            }
      }

const void * unused = realpath(argv0, APL_bin_path);
   APL_bin_path[PATH_MAX] = 0;
char * slash =   strrchr(APL_bin_path, '/');
   if (slash)   *slash = 0;

   // if we have a PWD and it is a prefix of APL_bin_path then replace PWD
   // by './'
   //
const char * PWD = getenv("PWD");
   if (PWD)   // we have a pwd
      {
        const int PWD_len = strlen(PWD);
        if (!strncmp(PWD, APL_bin_path, PWD_len) && PWD_len > 1)
           {
             strcpy(APL_bin_path + 1, APL_bin_path + PWD_len);
             APL_bin_path[0] = '.';
           }
      }
}
//-----------------------------------------------------------------------------
bool
LibPaths::is_lib_root(const char * dir)
{
char filename[PATH_MAX + 1];

   snprintf(filename, sizeof(filename), "%s/workspaces", dir);
   if (access(filename, F_OK))   return false;

   snprintf(filename, sizeof(filename), "%s/wslib1", dir);
   if (access(filename, F_OK))   return false;

   return true;
}
//-----------------------------------------------------------------------------
void
LibPaths::search_APL_lib_root()
{
const char * path = getenv("APL_LIB_ROOT");
   if (path)
      {
        const void * unused = realpath(path, APL_lib_root);
        root_from_env = true;
        return;
      }

   // search from "." to "/" for  a valid lib-root
   //
int last_len = 2*PATH_MAX;
   APL_lib_root[0] = '.';
   APL_lib_root[1] = 0;
   for (;;)
       {
         const void * unused = realpath(APL_lib_root, APL_lib_root);
         int len = strlen(APL_lib_root);

         if (is_lib_root(APL_lib_root))   return;   // lib-rot found

         // if length has not decreased then we stop in order to
         // avoid an endless loop.
         //
         if (len >= last_len)   break;
         last_len = len;

         // move up by appending /..
         //
         APL_lib_root[len++] = '/';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = 0;
       }

   // no lib-root found. use ".";
   //
const void * unused = realpath(".", APL_lib_root);
   root_from_pwd = true;
}
//-----------------------------------------------------------------------------
void
LibPaths::set_APL_lib_root(const char * new_root)
{
const void * unused = realpath(new_root, APL_lib_root);
}
//-----------------------------------------------------------------------------
void
LibPaths::set_lib_dir(LibRef libref, const char * path, LibDir::CfgSrc src)
{
   lib_dirs[libref].dir_path = UCS_string(path);
   lib_dirs[libref].cfg_src = src;
}
//-----------------------------------------------------------------------------
UCS_string
LibPaths::get_lib_dir(LibRef libref)
{
   switch(lib_dirs[libref].cfg_src)
      {
        case LibDir::CS_NONE: return UCS_string();

        case LibDir::CS_ENV:
        case LibDir::CS_ARGV0:     break;   // continue below

        case LibDir::CS_PREF_SYS:
        case LibDir::CS_PREF_HOME: return lib_dirs[libref].dir_path;

      }

UCS_string ret(APL_lib_root);
   if (libref == LIB0)
      {
        ret.append_utf8("/workspaces");
      }
   else
      {
        ret.append_utf8("/wslib");
        ret.append_number(libref);
      }

   return ret;
}
//-----------------------------------------------------------------------------
UTF8_string
LibPaths::get_lib_filename(LibRef lib, const UCS_string & name, 
                           bool existing, const char * extension)
{
   if (name[0] == UNI_ASCII_SLASH)   // absolute path
      {
         // absolute paths are fallbacks and never modified
         // but taken as they are
         //
         return UTF8_string(name);
      }

UCS_string ret = get_lib_dir(lib);
   ret.append_utf8("/");
   ret.append(name);

bool has_extension = false;

   if (existing)
      {
        // file ret is supposed to exist (and will be openend read-only).
        // If it does return filename otherwise filename.extension.
        //
        UTF8_string filename(ret);
        has_extension = access(filename.c_str(), F_OK) == 0;
      }
   else
      {
        // file may or may not exist (and will be created if not).
        // check that the file ends with extension and append extension if not.
        //
        const size_t elen = strlen(extension);
        bool has_extension = false;
       if (elen < name.size())   // name is long enough to end with .extension
          {
            has_extension = name[name.size() - elen] == UNI_ASCII_FULLSTOP;
            loop(e, elen)
               {
                 if (!has_extension)   break;
                 has_extension = name[name.size() - e] == Unicode(extension[e]);
               }
          }
      }

   if (!has_extension)
      {
        ret.append_utf8(".");
        ret.append_utf8(extension);
      }

   return UTF8_string(ret);
}
//-----------------------------------------------------------------------------

