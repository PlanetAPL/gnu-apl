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

#ifndef __LIBPATHS_HH_DEFINED__
#define __LIBPATHS_HH_DEFINED__

#include "UCS_string.hh"

//-----------------------------------------------------------------------------

/// a library reference number for commands )LOAD, )SAVE, and )COPY.
/// No library reference number is the same as LIB0.
enum LibRef
{
   LIB0 = 0,         ///< library 0
   LIB1 = 1,         ///< library 1
   LIB2 = 2,         ///< library 2
   LIB3 = 3,         ///< library 3
   LIB4 = 4,         ///< library 4
   LIB5 = 5,         ///< library 5
   LIB6 = 6,         ///< library 6
   LIB7 = 7,         ///< library 7
   LIB8 = 8,         ///< library 8
   LIB9 = 9,         ///< library 9
   LIB_MAX,          ///< valid library references are smaller than this
   LIB_NONE = LIB0   ///< no library reference specified.
};
//-----------------------------------------------------------------------------
/// a class mapping library reference numbers to directories
class LibPaths
{
public:
   struct LibDir
      {
        /// constructor: unspecified LibDir
        LibDir()
        : cfg_src(CS_NONE)
        {}

        /// the path (name) of the lib directory
        UCS_string dir_path;

        /// how a dir_path was computed
        enum CfgSrc
           {
             CS_NONE      = 0,   ///< not at all
             CS_ENV       = 1,   ///< lib root from env. variable APL_LIB_ROOT
             CS_ARGV0     = 2,   ///< lib root from apl binary location
             CS_PREF_SYS  = 3,   ///< path from preferences file below /etc/
             CS_PREF_HOME = 4,   ///< path from preferences file below $HOME
           };

        /// how dir_path was computed
        CfgSrc cfg_src;
      };

   /// initialize library paths based on the location of the APL binary
   static void init(const char * argv0);

   /// return the path (directory and filename) of the APL interpreter binary
   static const char * get_APL_bin_path()   { return APL_bin_path; }

   /// return directory containing (file or directory) workspaces and wslib1
   static const char * get_APL_lib_root()   { return APL_lib_root; }

   /// set library root to \b new_root
   static void set_APL_lib_root(const char * new_root);

   /// set library path (from config file)
   static void set_lib_dir(LibRef lib, const char * path, LibDir::CfgSrc src);

   /// return library path (from config file or from libroot)
   static UCS_string get_lib_dir(LibRef lib);

   /// return source that configured \b this entry
   static LibDir::CfgSrc get_cfg_src(LibRef lib)
      { return lib_dirs[lib].cfg_src; }

   /// return full path for file \b name
   static UTF8_string get_lib_filename(LibRef lib, const UCS_string & name,
                                      bool existing, const char * extension);
protected:
   /// compute the location of the apl binary
   static void compute_bin_path(const char * argv0);

   /// set library root, searching from APL_bin_path
   static void search_APL_lib_root();

   /// return true if directory \b dir contains two (files or sub-directories)
   /// workspaces and wslib1
   static bool is_lib_root(const char * dir);

   /// path (directory and filename) of the APL interpreter binary
   static char APL_bin_path[];

   /// a directory containing sub-directories workspaces and wslib1
   static char APL_lib_root[];

   /// directories for each library reference as specified in file preferences
   static LibDir lib_dirs[LIB_MAX];

   /// true if APL_lib_root was computed from environment variable APL_LIB_ROOT
   static bool root_from_env;

   /// true if APL_lib_root was not found ("." taken)
   static bool root_from_pwd;
};
//-----------------------------------------------------------------------------
#endif // __LIBPATHS_HH_DEFINED__
