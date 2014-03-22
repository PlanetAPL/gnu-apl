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

#ifndef __LINE_TESTFILES_HH_DEFINED__
#define __LINE_TESTFILES_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "Assert.hh"
#include "UTF8_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (cin)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) |
     +---+---+             +---------+---------+
         |                           |
         |                           |
         V                           |
      +--+--+                        |
      | APL |                        |
      +--+--+                        |
         |                           |
         |                           |
         +-----------------------+   |
         |                       |   |
         |                       V   V
         |                    +--+---+--+
         |                    | compare |
         |                    +----+----+
         |                         |
         |                         |
         V                         V
       (cout)                (test results)
 
 */

/** handling of testcase files. TestFiles reads lines from testfiles until they
    are all executed. The output of the APL interpreter is compared against
    the testcase files and mismatches are recorded.
 */
class TestFiles
{
   friend int main(int argc, const char *argv[]);

public:
   /// randomize the order of test_file_names
   static void randomize_files();

   /// the files to be processed.
   static vector<const char *> test_file_names;

   /// return true if this workspaces is in test mode
   static bool is_validating()   { return test_file_names.size() > 0; }

   /// count an error
   static void reset_errors()
      {
        apl_errors = 0;
        assert_errors = 0;
        diff_errors = 0;
        parse_errors = 0;
      }

   /// return the total number of errors
   static int error_count()
      { return apl_errors + assert_errors + diff_errors + parse_errors; }

   /// count and report a parse error
   static void syntax_error();

   /// reset APL errors, expecting cnt
   static void expect_apl_errors(const UCS_string & arg);

   /// count an APL error
   static void apl_error(const char * loc);

   /// count a failed assertion
   static void assert_error();

   /// count a output diff error
   static void diff_error();

   /// get one line from the current testcase file.
   /// open the next testcase file if necceessary.
   static const UTF8 * get_testcase_line() ;

   /// open the next test file 
   static void open_next_testfile();

   /// return the current test report
   static ofstream & get_current_testreport()   { return current_testreport; }

   /// return the name of the current test file
   static const char * get_testfile_name()   { return current_testfile_name; }

   /// return the current test file
   static FILE * get_testfile()   { return current_testfile; }

   /// return line number in the current test file
   static int get_current_lineno()   { return current_lineno; }

   /// read one line from input file with CR and LF removed
   static const UTF8 * read_file_line();

protected:
   /// how to handle test results
   static enum TestMode
      {
        /// exit() after last testcase file
        TM_EXIT_AFTER_LAST       = 0,

        /// exit() after last testcase file if no error
        TM_EXIT_AFTER_LAST_IF_OK = 1,

        /// continue in APL interpreter after last testcase file
        TM_STAY_AFTER_LAST       = 2,

        /// stop test execution after the first error (stay in APL interpreter)
        TM_STOP_AFTER_ERROR      = 3,

        /// exit() after the first error
        TM_EXIT_AFTER_ERROR      = 4,
      } test_mode;   /// the desired test mode as per argv[]

   /// write testcases summary file
   static void print_summary();

   /// true until total error count is printed.
   static bool need_total;

   /// the name of the current test file
   static const char * current_testfile_name;

   /// the current test file
   static FILE * current_testfile;

   /// line number in the current test file
   static int current_lineno;

   /// the number of testcases provided
   static int testcase_count;

   /// the number of testcases executed
   static int testcases_done;

   /// the number of parse errors when executing test file
   static int parse_errors;

   /// the tital number of errors in all test files
   static int total_errors;

   /// the number of APL errors when executing test file
   static int apl_errors;

   /// the source location where the last APL error was thrown
   static const char * last_apl_error_loc;

   /// the testcase file line that has triggered the last APL error
   static int last_apl_error_line;

   /// the number of output differences when executing test file
   static int diff_errors;

   /// the number of failed assertions when executing test file
   static int assert_errors;

   /// the current test report
   static ofstream current_testreport;
};

#endif // __LINE_TESTFILES_HH_DEFINED__
