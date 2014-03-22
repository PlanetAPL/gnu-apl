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

#include "CDR_string.hh"
#include "CharCell.hh"
#include "Common.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "main.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "SystemVariable.hh"
#include "TestFiles.hh"
#include "UCS_string.hh"
#include "Value.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"

ShapeItem Value::value_count = 0;
ShapeItem Value::total_ravel_count = 0;

// the static Value instances are defined in StaticObjects.cc

//-----------------------------------------------------------------------------
void
Value::init_ravel()
{
   owner_count = 0;

   ++value_count;
   if (Quad_SYL::value_count_limit &&
       Quad_SYL::value_count_limit < value_count)
      {
        --value_count;

        // make sure that the value is properly initialized
        //
        ravel = short_value;
        new (&shape) Shape();
        new (ravel)   IntCell(42);

        Workspace::more_error() = UCS_string(
        "the system limit on the APL value count (as set in ⎕SYL) was reached\n"
        "(and to avoid lock-up, the limit in ⎕SYL was automatically cleared).");

        // reset the limit so that we don't get stuck here.
        //
        Quad_SYL::value_count_limit = 0;
        attention_raised = interrupt_raised = true;
      }

const ShapeItem length = shape.get_volume();

   if (length > SHORT_VALUE_LENGTH_WANTED)
      {
        total_ravel_count += length;
        if (Quad_SYL::ravel_count_limit &&
            Quad_SYL::ravel_count_limit < total_ravel_count)
           {
             total_ravel_count -= length;

             // make sure that the value is properly initialized
             //
             ravel = short_value;
             new (&shape) Shape();
             new (ravel)   IntCell(42);

             Workspace::more_error() = UCS_string(
             "the system limit on the total ravel size (as set in ⎕SYL) "
             "was reached\n(and to avoid lock-up, the limit in ⎕SYL "
             "was automatically cleared).");

             // reset the limit so that we don't get stuck here.
             //
             Quad_SYL::ravel_count_limit = 0;
             attention_raised = interrupt_raised = true;
           }

        try
           {
             ravel = new Cell[length];
           }
        catch (...)
           {
             // for some unknown reason, this object gets cleared after
             // throwing the E_WS_FULL below (which destroys the prev and
             // next pointers. We therefore unlink() the object here (where
             // prev and next pointers are still intact).
             //
             unlink();
             throw_apl_error(E_WS_FULL, alloc_loc);
           }
      }
   else
      {
        ravel = short_value;
      }

   check_ptr = (const char *)this + 7;
}
//-----------------------------------------------------------------------------
Value::Value(const char * loc)
   : DynamicObject(loc, &all_values),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//-----------------------------------------------------------------------------
Value::Value(const char * loc, Value_how how)
   : DynamicObject(loc),
     flags(VF_forever),
     valid_ravel_items(0)
{
   // default shape is skalar
   //
   ADD_EVENT(this, VHE_Create, 0, loc);
   switch(how)
      {
        case Value_how_Zero ... Value_how_Nine:
             init_ravel();
             new (&get_ravel(0)) IntCell(how);
             break;

        case Value_how_Minus_One:
             init_ravel();
             new (&get_ravel(0)) IntCell(-1);
             break;

        case Value_how_Spc:
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             break;

        case Value_how_Idx0:
             new (&shape) Shape(0);
             init_ravel();
             new (&get_ravel(0)) IntCell(0);
             break;

        case Value_how_Str0:
             new (&shape) Shape(0);
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             break;

        case Value_how_zStr0:
             init_ravel();
             {
               Value_P z(new Value(loc, Value_how_Str0));   // ''
               new (&get_ravel(0)) PointerCell(z);               // ⊂''
             }
             break;

        case Value_how_Str0_0:
             new (&shape) Shape(ShapeItem(0), ShapeItem(0));
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             break;

        case Value_how_Str3_0:
             new (&shape) Shape(ShapeItem(3), ShapeItem(0));
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
             break;

        case Value_how_Max:
             init_ravel();
             new (&get_ravel(0)) FloatCell(BIG_FLOAT);
             break;

        case Value_how_Min:
             init_ravel();
             new (&get_ravel(0)) FloatCell(-BIG_FLOAT);
             break;

        case Value_how_AV:
             new (&shape) Shape(MAX_AV);
             init_ravel();
             loop(cti, MAX_AV)
                 {
                   const int pos = Avec::get_av_pos(CHT_Index(cti));
                   const Unicode uni = Avec::unicode(CHT_Index(cti));

                   new (&get_ravel(pos))  CharCell(uni);
                 }
             break;

        case Value_how_Max_CT:
             init_ravel();
             new (&get_ravel(0)) FloatCell(MAX_QUAD_CT);
             break;

        case Value_how_Max_PP:
             init_ravel();
             new (&get_ravel(0)) IntCell(MAX_QUAD_PP);
             break;

        case Value_how_Quad_NLT:
             {
                const char * cp = "En_US.nlt";
                const ShapeItem len = strlen(cp);
                new (&shape) Shape(len);
                init_ravel();
                loop(l, len)   new (&get_ravel(l)) CharCell(Unicode(cp[l]));
             }
             break;

        case Value_how_Quad_TC:
             new (&shape) Shape(3);
             init_ravel();
             new (&get_ravel(0)) CharCell(UNI_ASCII_BS);
             new (&get_ravel(1)) CharCell(UNI_ASCII_CR);
             new (&get_ravel(2)) CharCell(UNI_ASCII_LF);
             break;
      }

   check_value(LOC);
   owner_count = 1;
}
//-----------------------------------------------------------------------------
Value::Value(ShapeItem sh, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(sh),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//-----------------------------------------------------------------------------
Value::Value(const UCS_string & ucs, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(ucs.size()),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   if (ucs.size())
      {
        loop(l, ucs.size())   new (&get_ravel(l)) CharCell(ucs[l]);
      }
   else   // empty string: set prototype
      {
        new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
      }

   set_complete();
}
//-----------------------------------------------------------------------------
Value::Value(const CDR_string & ui8, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(ui8.size()),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   if (ui8.size())
      {
        loop(l, ui8.size())   new (&get_ravel(l)) CharCell(Unicode(ui8[l]));
      }
   else   // empty string: set prototype
      {
        new (&get_ravel(0)) CharCell(UNI_ASCII_SPACE);
      }

   set_complete();
}
//-----------------------------------------------------------------------------
Value::Value(const char * string, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(strlen(string)),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

const ShapeItem length = strlen(string);
   loop(e, length)  new (&get_ravel(e)) CharCell(Unicode(string[e]));

   set_complete();
}
//-----------------------------------------------------------------------------
Value::Value(const Shape & sh, const char * loc)
   : DynamicObject(loc, &all_values),
     shape(sh),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//-----------------------------------------------------------------------------
Value::Value(const Cell & cell, const char * loc)
   : DynamicObject(loc, &all_values),
     flags(VF_NONE),
     valid_ravel_items(0)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   get_ravel(0).init(cell);
   check_value(LOC);
}
//-----------------------------------------------------------------------------
Value::~Value()
{
   ADD_EVENT(this, VHE_Destruct, 0, LOC);
   unlink();

   release_sub(LOC);

const ShapeItem length = nz_element_count();
   --value_count;

   if (ravel != short_value)
      {
        total_ravel_count -= length;
        delete ravel;
      }

   Assert(check_ptr == (const char *)this + 7);
   check_ptr = 0;
}
//-----------------------------------------------------------------------------
void
Value::release_sub(const char * loc)
{
const ShapeItem length = nz_element_count();

Cell * cZ = &get_ravel(0);
   loop(c, length)   cZ++->release(loc);
}
//-----------------------------------------------------------------------------
Value_P
Value::get_cellrefs(const char * loc)
{
Value_P ret(new Value(get_shape(), loc));
   ret->set_left();

const ShapeItem ec = nz_element_count();

   loop(e, ec)
      {
        Cell & cell = get_ravel(e);
        new (ret->next_ravel())   LvalCell(&cell);
      }

   ret->check_value(LOC);
   return ret;
}
//-----------------------------------------------------------------------------
void
Value::assign_cellrefs(Value_P new_value)
{
const ShapeItem dest_count = nz_element_count();
const ShapeItem value_count = new_value->nz_element_count();
const Cell * src = &new_value->get_ravel(0);
Cell * C = &get_ravel(0);

const int src_incr = new_value->is_skalar() ? 0 : 1;

   if (is_skalar())
      {
        if (!C->is_lval_cell())   LEFT_SYNTAX_ERROR;
        Cell * dest = C->get_lval_value();   // can be 0!
        if (dest)   dest->release(LOC);   // free sub-values etc (if any)

        if (new_value->is_skalar())
           {
             dest->init(new_value->get_ravel(0));
           }
        else
           {
             new (dest)   PointerCell(new_value);
           }
        return;
      }


   loop(d, dest_count)
      {
        if (!C->is_lval_cell())   LEFT_SYNTAX_ERROR;

        Cell * dest = C++->get_lval_value();   // can be 0!
        if (dest)   dest->release(LOC);   // free sub-values etc (if any)

        // erase the pointee when overriding a pointer-cell.
        //
        if (dest)   dest->init(*src);
        src += src_incr;
      }
}
//-----------------------------------------------------------------------------
bool
Value::is_lval() const
{
const ShapeItem ec = nz_element_count();

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_lval_cell())   return true;
        if (cell.is_pointer_cell() && cell.get_pointer_value()->is_lval())
           return true;

        return false;
      }

   return false;
}
//-----------------------------------------------------------------------------
void
Value::flag_info(const char * loc, ValueFlags flag, const char * flag_name,
                 bool set) const
{
const char * sc = set ? " SET " : " CLEAR ";
const int new_flags = set ? flags | flag : flags & ~flag;
const char * chg = flags == new_flags ? " (no change)" : " (changed)";

   CERR << "Value " << (const void *)this
        << sc << flag_name << " (" << (const void *)flag << ")"
        << " at " << loc << " now = " << (const void *)(intptr_t)new_flags
        << chg << endl;
}
//-----------------------------------------------------------------------------
void
Value::init()
{
   Log(LOG_startup)
      CERR << "Max. Rank            is " << MAX_RANK << endl
           << "sizeof(Value header) is " << sizeof(Value)  << " bytes" << endl
           << "Cell size            is " << sizeof(Cell)   << " bytes" << endl;
};
//-----------------------------------------------------------------------------
void
Value::mark_all_dynamic_values()
{
   for (const DynamicObject * dob = DynamicObject::all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         if (!val->is_forever())   val->set_marked();
       }
}
//-----------------------------------------------------------------------------
void
Value::unmark() const
{
   clear_marked();

const ShapeItem ec = nz_element_count();
const Cell * C = &get_ravel(0);
   loop(e, ec)
      {
        if (C->is_pointer_cell())   C->get_pointer_value()->unmark();
        ++C;
      }
}
//-----------------------------------------------------------------------------
void
Value::rollback(ShapeItem items, const char * loc)
{
   ADD_EVENT(this, VHE_Unroll, 0, loc);

   release_sub(loc);

   loop(i, items)
      {
        Cell & cell = get_ravel(i);
        cell.release(LOC);
      }

   ((Value *)this)->alloc_loc = loc;
}
//-----------------------------------------------------------------------------
void
Value::erase_all(ostream & out)
{
   for (DynamicObject * vb = DynamicObject::all_values.get_next();
        vb != &DynamicObject::all_values; vb = vb->get_next())
       {
         Value * v = (Value *)vb;
         out << "erase_all sees Value:" << endl
             << "  Allocated by " << v->where_allocated() << endl
             << "  ";
         v->list_one(CERR, false);
       }
}
//-----------------------------------------------------------------------------
int
Value::erase_stale(const char * loc)
{
int count = 0;

   Log(LOG_Value__erase_stale)
      CERR << endl << endl << "erase_stale() called from " << loc << endl;

   for (DynamicObject * obj = all_values.get_next();
        obj != &all_values; obj = obj->get_next())
       {
         if (obj == obj->get_next())   // a loop
            {
              CERR << "A loop in DynamicObject::all_values (detected in "
                      "function erase_stale() at object "
                   << (const void *)obj << "): " << endl;
              all_values.print_chain(CERR);
              CERR << endl;

              CERR << " DynamicObject: " << obj << endl;
              CERR << " Value:         " << (Value *)obj << endl;
              CERR << *(Value *)obj << endl;
            }

         Assert(obj != obj->get_next());
         Value * v = (Value *)obj;
         if (v->owner_count)   continue;

         ADD_EVENT(v, VHE_Stale, v->owner_count, loc);

         Log(LOG_Value__erase_stale)
            {
              CERR << "Erasing stale Value " << (const void *)obj << ":" << endl
                   << "  Allocated by " << v->where_allocated() << endl
                   << "  ";
              v->list_one(CERR, false);
            }

         // count it unless we know it is dirty
         //
         ++count;

         obj->unlink();

         // v->erase(loc) could mess up the chain, so we start over
         // rather than continuing
         //
         obj = &all_values;
       }

   return count;
}
//-----------------------------------------------------------------------------
int
Value::finish_incomplete(const char * loc)
{
   // finish_incomplete() is called when a function, typically one of the
   // StateIndicator::eval_XXX() functions, has thrown an error.
   // The value has been constructed, its shape is correct, but some (or all)
   // of the ravel cells are uninitialized. The VF_complete bit of such
   // values is not set.
   //
   // We fix this by initializing the entire ravel to 42424242. The value
   // probably remains stale, though. Deleting it here could cause double-
   // delete problems, so we rather set the dirty bit.
   //
int count = 0;

   Log(LOG_Value__erase_stale)
      {
        CERR << endl << endl
             << "finish_incomplete() called from " << loc << endl;
      }

   for (DynamicObject * obj = all_values.get_next();
        obj != &all_values; obj = obj->get_next())
       {
         if (obj == obj->get_next())   // a loop
            {
              CERR << "A loop in DynamicObject::all_values (detected in "
                      "function Value::finish_incomplete() at object "
                   << (const void *)obj << "): " << endl;
              all_values.print_chain(CERR);
              CERR << endl;

              CERR << " DynamicObject: " << obj << endl;
              CERR << " Value:         " << (Value *)obj << endl;
              CERR << *(Value *)obj << endl;
            }

         Assert(obj != obj->get_next());
         Value * v = (Value *)obj;
         if (v->flags & VF_complete)   continue;

         ADD_EVENT(v, VHE_Completed, v->owner_count, LOC);

         ShapeItem ravel_length = v->nz_element_count();
         Cell * cv = &v->get_ravel(0);
         loop(r, ravel_length)   new (cv++)   IntCell(42424242);
         v->set_complete();

         Log(LOG_Value__erase_stale)
            {
              CERR << "Fixed incomplete Value "
                   << (const void *)obj << ":" << endl
                   << "  Allocated by " << v->where_allocated() << endl
                   << "  ";
              v->list_one(CERR, false);
            }

         v->set_dirty();
         ++count;
       }

   return count;
}
//-----------------------------------------------------------------------------
ostream &
Value::list_one(ostream & out, bool show_owners)
{
   if (flags)
      {
        out << "   Flags =";
        char sep = ' ';
        if (is_forever())    { out << sep << "FOREVER";    sep = '+'; }
        if (is_left())       { out << sep << "LEFT";       sep = '+'; }
        if (is_dirty())      { out << sep << "DIRTY";      sep = '+'; }
        if (is_complete())   { out << sep << "COMPLETE";   sep = '+'; }
        if (is_marked())     { out << sep << "MARKED";     sep = '+'; }
        out << ",";
      }
   else
      {
        out << "   Flags = NONE,";
      }

   out << " Rank = " << get_rank();
   if (get_rank())
      {
        out << ", Shape =";
        loop(r, get_rank())   out << " " << get_shape_item(r);
      }

   out << ":" << endl;
   print(out);
   out << endl;
   if (!show_owners)   return out;

   // print owners...
   //
   out << "Owners of " << (const void *)(intptr_t)this << ":" << endl;

   // static variables
   //
#define stv_def(x) \
   if (this == x ## _P.get())   out << "(static) Value::_" #x << endl;
#include "StaticValues.def"

   Workspace::show_owners(out, *this);

   out << "---------------------------" << endl << endl;
   return out;
}
//-----------------------------------------------------------------------------
ostream &
Value::list_all(ostream & out, bool show_owners)
{
int num = 0;
   for (DynamicObject * vb = all_values.get_prev();
        vb != &all_values; vb = vb->get_prev())
       {
         out << "Value #" << num++ << ":";
         ((Value *)vb)->list_one(out, show_owners);
       }

   return out << endl;
}
//-----------------------------------------------------------------------------
ShapeItem
Value::get_enlist_count() const
{
const ShapeItem ec = element_count();
ShapeItem count = ec;

   loop(c, ec)
       {
         const Cell & cell = get_ravel(c);
         if (cell.is_pointer_cell())
            {
               count--;   // the pointer cell
               count += cell.get_pointer_value()->get_enlist_count();
            }
         else if (cell.is_lval_cell())
            {
              Cell * cp = cell.get_lval_value();
              if (cp && cp->is_pointer_cell())
                 {
                   count--;
                   count += cp->get_pointer_value()->get_enlist_count();
                 }
            }
       }

   return count;
}
//-----------------------------------------------------------------------------
void
Value::enlist(Cell * & dest, bool left) const
{
ShapeItem ec = element_count();

   loop(c, ec)
       {
         const Cell & cell = get_ravel(c);
         if (cell.is_pointer_cell())
            {
              cell.get_pointer_value()->enlist(dest, left);
            }
         else if (left && cell.is_lval_cell())
            {
              const Cell * cp = cell.get_lval_value();
              if (cp == 0)
                 {
                   CERR << "0-pointer at " LOC << endl;
                 }
              else if (cp->is_pointer_cell())
                 {
                   cp->get_pointer_value()->enlist(dest, left);
                 }
              else 
                 {
                   new (dest++) LvalCell(cell.get_lval_value());
                 }
            }
         else if (left)
            {
              new (dest++) LvalCell((Cell *)&cell);
            }
         else
            {
              dest++->init(cell);
            }
       }
}
//-----------------------------------------------------------------------------
bool
Value::is_apl_char_vector() const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
      {
        if (!get_ravel(c).is_character_cell())   return false;

        const Unicode uni = get_ravel(c).get_char_value();
        if (Avec::find_char(uni) == Invalid_CHT)   return false;   // not in ⎕AV
      }
   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_char_vector() const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
      if (!get_ravel(c).is_character_cell())   return false;
   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_char_skalar() const
{
   if (get_rank() != 0)   return false;

   return get_ravel(0).is_character_cell();
}
//-----------------------------------------------------------------------------
bool
Value::NOTCHAR() const
{
   // always test element 0.
   if (!get_ravel(0).is_character_cell())   return true;

const ShapeItem ec = element_count();
   loop(c, ec)   if (!get_ravel(c).is_character_cell())   return true;

   // all items are single chars.
   return false;
}
//-----------------------------------------------------------------------------
int
Value::toggle_UCS()
{
const ShapeItem ec = nz_element_count();
int error_count = 0;

   loop(e, ec)
      {
        Cell & cell = get_ravel(e);
        if (cell.is_character_cell())
           {
             new (&cell)   IntCell(Unicode(cell.get_char_value()));
           }
        else if (cell.is_integer_cell())
           {
             new (&cell)   CharCell(Unicode(cell.get_int_value()));
           }
        else if (cell.is_pointer_cell())
           {
             error_count += cell.get_pointer_value()->toggle_UCS();
           }
        else
           {
             ++error_count;
           }
      }

   return error_count;
}
//-----------------------------------------------------------------------------
bool
Value::is_int_vector(APL_Float qct) const
{
   if (get_rank() != 1)   return false;

   loop(c, get_shape_item(0))
       {
         const Cell & cell = get_ravel(c);
         if (!cell.is_near_int(qct))   return false;
       }

   return true;
}
//-----------------------------------------------------------------------------
bool
Value::is_int_skalar(APL_Float qct) const
{
   if (get_rank() != 0)   return false;

   return get_ravel(0).is_near_int(qct);
}
//-----------------------------------------------------------------------------
bool
Value::is_complex(APL_Float qct) const
{
const ShapeItem ec = nz_element_count();

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (!cell.is_numeric())        DOMAIN_ERROR;
        if (!cell.is_near_real(qct))   return true;
      }

   return false;   // all cells numeric and not complex
}
//-----------------------------------------------------------------------------
Depth
Value::compute_depth() const
{
   if (is_skalar())
      {
        if (get_ravel(0).is_pointer_cell())
           {
             Depth d = get_ravel(0).get_pointer_value()->compute_depth();
             return 1 + d;
           }

        return 0;
      }

ShapeItem count = nz_element_count();

Depth sub_depth = 0;
   loop(c, count)
       {
         Depth d = 0;
         if (get_ravel(c).is_pointer_cell())
            {
              d = get_ravel(c).get_pointer_value()->compute_depth();
            }
         if (sub_depth < d)   sub_depth = d;
       }

   return sub_depth + 1;
}
//-----------------------------------------------------------------------------
CellType
Value::compute_cell_types() const
{
int32_t ctypes = CT_NONE;

const int64_t count = element_count();
   loop(c, count)
      {
       const Cell & cell = get_ravel(c);
        ctypes |= cell.get_cell_type();
        if (cell.is_pointer_cell())
           ctypes |= cell.get_pointer_value()->compute_cell_types();
      }

   return CellType(ctypes);
}
//-----------------------------------------------------------------------------
Value_P
Value::index(const IndexExpr & IX) const
{
   Assert(IX.value_count() != 1);   // should have called index(Value_P X)

   if (get_rank() != IX.value_count())   RANK_ERROR;

   // Notes:
   //
   // 1.  IX is parsed from right to left:    B[I2;I1;I0]  --> I0 I1 I2
   //     the shapes of this and IX are then related as follows:
   //
   //     this     IX
   //     ---------------
   //     0        rank-1   (rank = IX->value_count())
   //     1        rank-2
   //     ...      ...
   //     rank-2   1
   //     rank-1   0
   //     ---------------
   //
   // 2.  shape Z is the concatenation of all shapes in IX
   // 3.  rank Z is the sum of all ranks in IX

   // construct result rank_Z and shape_Z.
   // We go from higher indices of IX to lower indices (Note 1.)
   //
Shape shape_Z;
   loop(this_r, get_rank())
       {
         const ShapeItem idx_r = get_rank() - this_r - 1;

         Value_P I = IX.values[idx_r];
         if (!!I)
            {
              loop(s, I->get_rank())
                shape_Z.add_shape_item(I->get_shape_item(s));
            }
         else
            {
              shape_Z.add_shape_item(this->get_shape_item(this_r));
            }
       }

MultiIndexIterator mult(get_shape(), IX);   // deletes IDX

Value_P Z(new Value(shape_Z, LOC));
const ShapeItem ec_z = Z->element_count();

   if (ec_z == 0)   // empty result
      {
        Z->set_default(*this);
        Z->check_value(LOC);
        return Z;
      }

   // construct iterators.
   // We go from lower indices to higher indices in IX, which
   // means from higher indices to lower indices in this and Z
   
   loop(z, ec_z)   Z->get_ravel(z).init(get_ravel(mult.next()));

   Assert(mult.done());

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Value::index(Value_P X) const
{
   if (get_rank() != 1)   RANK_ERROR;

   if (!X)   return clone(LOC);

const Shape shape_Z(X->get_shape());
Value_P Z(new Value(shape_Z, LOC));
const ShapeItem ec = shape_Z.get_volume();
const ShapeItem max_idx = element_count();
const APL_Integer qio = Workspace::get_IO();
const APL_Float qct = Workspace::get_CT();

const Cell * cI = &X->get_ravel(0);

   while (Z->more())
      {
         const ShapeItem idx = cI++->get_near_int(qct) - qio;
         if (idx < 0 || idx >= max_idx)
            {
              Z->rollback(Z->valid_ravel_items, LOC);
              INDEX_ERROR;
            }

         Z->next_ravel()->init(get_ravel(idx));
      }

   Z->set_default(*this);

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Value::index(Token & IX) const
{
   if (IX.get_tag() == TOK_AXES)     return index(IX.get_apl_val());
   if (IX.get_tag() == TOK_INDEX)    return index(IX.get_index_val());

   // not supposed to happen
   //
   Q(IX.get_tag());  FIXME;
}
//-----------------------------------------------------------------------------
Rank
Value::get_single_axis(Rank max_axis) const
{
   if (this == 0)   AXIS_ERROR;

const APL_Float   qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!is_skalar_or_len1_vector())     AXIS_ERROR;

   if (!get_ravel(0).is_near_int(qct))   AXIS_ERROR;

   // if axis becomes (signed) negative then it will be (unsigned) too big.
   // Therefore we need not test for < 0.
   //
const unsigned int axis = get_ravel(0).get_near_int(qct) - qio;
   if (axis >= max_axis)   AXIS_ERROR;

   return axis;
}
//-----------------------------------------------------------------------------
Shape
Value::to_shape() const
{
   if (this == 0)   INDEX_ERROR;   // elided index ?

const ShapeItem xlen = element_count();
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

Shape shape;
     loop(x, xlen)
        shape.add_shape_item(get_ravel(x).get_near_int(qct) - qio);

   return shape;
}
//-----------------------------------------------------------------------------
void
Value::glue(Token & result, Token & token_A, Token & token_B, const char * loc)
{
Value_P A = token_A.get_apl_val();
Value_P B = token_B.get_apl_val();

const bool strand_A = token_A.get_tag() == TOK_APL_VALUE3;
const bool strand_B = token_B.get_tag() == TOK_APL_VALUE3;

   if (strand_A)
      {
        if (strand_B)   glue_strand_strand(result, A, B, loc);
        else            glue_strand_closed(result, A, B, loc);
      }
   else
      {
        if (strand_B)   glue_closed_strand(result, A, B, loc);
        else            glue_closed_closed(result, A, B, loc);
      }
}
//-----------------------------------------------------------------------------
void
Value::glue_strand_strand(Token & result, Value_P A, Value_P B,
                          const char * loc)
{
   // glue two strands A and B
   //
const ShapeItem len_A = A->element_count();
const ShapeItem len_B = B->element_count();

   Log(LOG_glue)
      {
        CERR << "gluing strands " << endl << *A
             << "with shape " << A->get_shape() << endl
             << " and " << endl << *B << endl
             << "with shape " << B->get_shape() << endl;
      }

   Assert(A->is_skalar_or_vector());
   Assert(B->is_skalar_or_vector());

Value_P Z(new Value(len_A + len_B, LOC));

   loop(a, len_A)   Z->next_ravel()->init(A->get_ravel(a));
   loop(b, len_B)   Z->next_ravel()->init(B->get_ravel(b));

   Z->check_value(LOC);
   new (&result) Token(TOK_APL_VALUE3, Z);
}
//-----------------------------------------------------------------------------
void
Value::glue_strand_closed(Token & result, Value_P A, Value_P B,
                          const char * loc)
{
   // glue a strand A to new item B
   //
   Log(LOG_glue)
      {
        CERR << "gluing strand " << endl << *A
             << " to non-strand " << endl << *B << endl;
      }

   Assert(A->is_skalar_or_vector());

const ShapeItem len_A = A->element_count();
Value_P Z(new Value(len_A + 1, LOC));

   loop(a, len_A)   Z->next_ravel()->init(A->get_ravel(a));

   if (B->is_simple_skalar())
      {
        Z->next_ravel()->init(B->get_ravel(0));
      }
   else
      {
        new (Z->next_ravel()) PointerCell(B);
      }

   Z->check_value(LOC);
   new (&result) Token(TOK_APL_VALUE3, Z);
}
//-----------------------------------------------------------------------------
void
Value::glue_closed_strand(Token & result, Value_P A, Value_P B,
                          const char * loc)
{
   // glue a new item A to the strand B
   //
   Log(LOG_glue)
      {
        CERR << "gluing non-strand " << endl << *A
             << " to strand " << endl << *B << endl;
      }

   Assert(B->is_skalar_or_vector());

const ShapeItem len_B = B->element_count();
Value_P Z(new Value(len_B + 1, LOC));

   if (A->is_simple_skalar())
      {
        Z->next_ravel()->init(A->get_ravel(0));
      }
   else
      {
        new (Z->next_ravel()) PointerCell(A);
      }

   loop(b, len_B)   Z->next_ravel()->init(B->get_ravel(b));

   Z->check_value(LOC);
   new (&result) Token(TOK_APL_VALUE3, Z);
}
//-----------------------------------------------------------------------------
void
Value::glue_closed_closed(Token & result, Value_P A, Value_P B,
                          const char * loc)
{
   // glue two non-strands together, starting a strand
   //
   Log(LOG_glue)
      {
        CERR << "gluing two non-strands " << endl << *A
             << " and " << endl << *B << endl;
      }

Value_P Z(new Value(2, LOC));
   if (A->is_simple_skalar())
      {
        Z->next_ravel()->init(A->get_ravel(0));
      }
   else
      {
        new (Z->next_ravel()) PointerCell(A);
      }

   if (B->is_simple_skalar())
      {
        Z->next_ravel()->init(B->get_ravel(0));
      }
   else
      {
        new (Z->next_ravel()) PointerCell(B);
      }

   Z->check_value(LOC);
   new (&result) Token(TOK_APL_VALUE3, Z);
}
//-----------------------------------------------------------------------------
void
Value::check_value(const char * loc)
{
#ifdef VALUE_CHECK_WANTED

   // if value was initialized by means of the next_ravel() mechanism,
   // then all cells are supposed to be OK.
   //
   if (valid_ravel_items && valid_ravel_items >= element_count())   return;

uint32_t error_count = 0;
const CellType ctype = get_ravel(0).get_cell_type();
   switch(ctype)
      {
        case CT_CHAR:
        case CT_POINTER:
        case CT_CELLREF:
        case CT_INT:
        case CT_FLOAT:
        case CT_COMPLEX:   break;   // OK

        default:
                 CERR << endl
                      << "*** check_value(" << loc << ") detects:" << endl
                      << "   bad ravel[0] (CellType " << ctype << ")" << endl;
                 ++error_count;
      }

    loop(c, element_count())
       {
         const Cell * cell = &get_ravel(c);
         const CellType ctype = cell->get_cell_type();
         switch(ctype)
            {
              case CT_CHAR:
              case CT_POINTER:
              case CT_CELLREF:
              case CT_INT:
              case CT_FLOAT:
              case CT_COMPLEX:   break;   // OK

              default:
                 CERR << endl
                      << "*** check_value(" << loc << ") detects:"
                      << endl
                      << "   bad ravel[" << c << "] (CellType "
                      << ctype << ")" << endl;
                 ++error_count;
            }

         if (error_count >= 10)
            {
              CERR << endl << "..." << endl;
              break;
            }
       }

   if (error_count)
      {
        CERR << "Shape: " << get_shape() << endl;
        print(CERR) << endl
           << "************************************************"
           << endl;
        Assert(0 && "corrupt ravel");
      }
#endif

   set_complete();
}
//-----------------------------------------------------------------------------
int
Value::total_size_netto(int CDR_type) const
{
   if (CDR_type != 7)   // not nested
      return 16 + 4*get_rank() + data_size(CDR_type);

   // nested: header + offset-array + sub-values.
   //
const ShapeItem ec = nz_element_count();
int size = 16 + 4*get_rank() + 4*ec;   // top_level size
   size = (size + 15) & ~15;           // rounded up to 16 bytes

   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_character_cell() || cell.is_numeric())
           {
             // a non-pointer sub value consisting of its own 16 byte header,
             // and 1-16 data bytes, padded up to 16 bytes,
             //
             size += 32;
           }
        else if (cell.is_pointer_cell())
           {
             Value_P sub_val = cell.get_pointer_value();
             const int sub_type = sub_val->get_CDR_type();
             size += sub_val->total_size_brutto(sub_type);
           }
         else
           DOMAIN_ERROR;
      }

   return size;
}
//-----------------------------------------------------------------------------
int
Value::data_size(int CDR_type) const
{
const ShapeItem ec = nz_element_count();
   
   switch (CDR_type) 
      {
        case 0: return (ec + 7) / 8;   // 1/8 byte bit, rounded up
        case 1: return 4*ec;           //   4 byte integer
        case 2: return 8*ec;           //   8 byte float
        case 3: return 16*ec;          // two 8 byte floats
        case 4: return ec;             // 1 byte char
        case 5: return 4*ec;           // 4 byte Unicode char
        case 7: break;                 // nested: continue below.
             
        default: FIXME;
      }      
             
   // compute size of a nested CDR.
   // The top level consists of structural offsets that do not count as data.
   // We therefore simly add up the data sizes for the sub-values.
   //
int size = 0;
   loop(e, ec)
      {
        const Cell & cell = get_ravel(e);
        if (cell.is_character_cell() || cell.is_numeric())
           {
             size += cell.CDR_size();
           }
        else if (cell.is_pointer_cell())
           {
             Value_P sub_val = cell.get_pointer_value();
             const int sub_type = sub_val->get_CDR_type();
             size += sub_val->data_size(sub_type);
           }
         else
           DOMAIN_ERROR;
      }

   return size;
}
//-----------------------------------------------------------------------------
int
Value::get_CDR_type() const
{
const ShapeItem ec = nz_element_count();
const Cell & cell_0 = get_ravel(0);

   // if all cells are characters (8 or 32 bit), then return 4 or 5.
   // if all cells are numeric (1, 32, 64, or 128 bit, then return  0 ... 3
   // otherwise return 7 (nested) 

   if (cell_0.is_character_cell())   // 8 or 32 bit characters.
      {
        bool big = false;   // assume 8-bit char
        loop(e, ec)
           {
             const Cell & cell = get_ravel(e);
             if (!cell.is_character_cell())   return 7;
             const Unicode uni = cell.get_char_value();
             if (uni < 0)      big = true;
             if (uni >= 256)   big = true;
           }

        return big ? 5 : 4;   // 8-bit or 32-bit char
      }

   if (cell_0.is_numeric())
      {
        bool is_bool    = false;
        bool is_int     = false;
        bool is_float   = false;
        bool is_complex = false;

        loop(e, ec)
           {
             const Cell & cell = get_ravel(e);
             if (cell.is_integer_cell())
                {
                  const APL_Integer i = cell.get_int_value();
                  const uint32_t high = i >> 32;
                  if (i == 0)                    is_bool  = true;
                  else if (i == 1)               is_bool  = true;
                  else if (high == 0x00000000)   is_int   = true;
                  else if (high == 0xFFFFFFFF)   is_int   = true;
                  else /* too big for int32 */   is_float = true;
                }
             else if (cell.is_float_cell())
                {
                  is_float  = true;
                }
             else if (cell.is_complex_cell())
                {
                  is_complex  = true;
                }
             else return 7;   // mixed: return 7
           }

        if (is_complex)   return 3;
        if (is_float)     return 2;
        if (is_int)       return 1;
        return 0;
      }

   return 7;
}
//-----------------------------------------------------------------------------
ostream &
Value::print(ostream & out) const
{
PrintStyle style;
   if (get_rank() < 2)   // skalar or vector
      {
        style = PR_APL_MIN;
      }
   else                  // matrix or higher
      {
        style = Workspace::get_PS();
        style = PrintStyle(style | PST_NO_FRACT_0);
      }

PrintContext pctx(style, Workspace::get_PP(), 
                         Workspace::get_CT(), Workspace::get_PW());
PrintBuffer pb(*this, pctx);

//   pb.debug(CERR, "Value::print()");

UCS_string ucs(pb, get_rank());

   if (ucs.size() == 0)   return out;

   return out << ucs << endl;
}
//-----------------------------------------------------------------------------
ostream &
Value::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Addr:    " << (const void *)this << endl
       << ind << "Rank:    " << get_rank()  << endl
       << ind << "Shape:   " << get_shape() << endl
       << ind << "Flags:   " << get_flags();
   if (is_forever())    out << " VF_forever";
   if (is_left())       out << " VF_left";
   if (is_dirty())      out << " VF_dirty";
   if (is_complete())   out << " VF_complete";
   if (is_marked())     out << " VF_marked";
   out << endl
       << ind << "First:   " << get_ravel(0)  << endl
       << ind << "Dynamic: ";

   DynamicObject::print(out);
   return out;
}
//-----------------------------------------------------------------------------
void
Value::debug(const char * info)
{
const PrintContext pctx;
PrintBuffer pb(*this, pctx);
   pb.debug(CERR, info);
}
//-----------------------------------------------------------------------------
ostream &
Value::print_boxed(ostream & out, const char * info)
{
   if (info)   out << info << endl;

Value_P Z = Quad_CR::fun.do_CR(4, *this);
   out << *Z << endl;
   return out;
}
//-----------------------------------------------------------------------------
UCS_string
Value::get_UCS_ravel() const
{
UCS_string ucs;

const ShapeItem ec = element_count();
   loop(e, ec)   ucs.append(get_ravel(e).get_char_value());

   return ucs;
}
//-----------------------------------------------------------------------------
void
Value::to_varnames(vector<UCS_string> & result, bool last) const
{
const ShapeItem var_count = get_rows();
const ShapeItem name_len = get_cols();

   loop(v, var_count)
      {
        ShapeItem nidx = v*name_len;
        UCS_string & name = result[v];
        loop(n, name_len)
           {
             const Unicode uni = get_ravel(nidx++).get_char_value();

             if (n == 0 && Avec::is_quad(uni))   // leading ⎕
                {
                  name.append(uni);
                }
             else if (Avec::is_symbol_char(uni))   // valid symbol char
                {
                  name.append(uni);
                }
             else if (last)
                {
                  const ShapeItem end = v*name_len;
                  while (nidx < end && get_ravel(nidx++).get_char_value()
                         == UNI_ASCII_SPACE)   ++nidx;
                  if (nidx == end)   break;   // remaining chars are spaces

                  // another name following
                  name.clear();
                  name.append(get_ravel(nidx++).get_char_value());
                }
             else  
                {
                  break;
                }
           }
      }
}
//-----------------------------------------------------------------------------
void
Value::to_proto()
{
const ShapeItem ec = nz_element_count();
Cell * c = &get_ravel(0);

   loop(e, ec)
      {
        if (c->is_pointer_cell())   c->get_pointer_value()->to_proto();
        else                        new (c) IntCell(0);
        ++c;
      }
}
//-----------------------------------------------------------------------------
void
Value::print_structure(ostream & out, int indent, ShapeItem idx) const
{
   loop(i, indent)   out << "    ";
   if (indent)   out << "[" << idx << "] ";
   out << "addr=" << (const void *)this
       << " ≡" << compute_depth()
       << " ⍴" << get_shape()
       << " flags: " << HEX4(get_flags()) << "   "
       << get_flags()
       << " " << where_allocated()
       << endl;

const ShapeItem ec = nz_element_count();
const Cell * c = &get_ravel(0);
   loop(e, ec)
      {
        if (c->is_pointer_cell())
           c->get_pointer_value()->print_structure(out, indent + 1, e);
        ++c;
      }
}
//-----------------------------------------------------------------------------
Value_P
Value::prototype(const char * loc) const
{
   // the type of an array is an array with the same structure, but all numbers
   // replaced with 0 and all chars replaced with ' '.
   //
   // the prototype of an array is the type of the first element of the array.

const Cell & first = get_ravel(0);

   if (first.is_pointer_cell())
      {
        Value_P B0 = first.get_pointer_value();
        Value_P Z(new Value(B0->get_shape(), loc));
        const ShapeItem ec_Z =  Z->nz_element_count();

        loop(z, ec_Z)   Z->get_ravel(z).init_type(B0->get_ravel(z));
        return Z;
      }
   else
      {
        Value_P Z(new Value(loc));

        Z->get_ravel(0).init_type(first);
        return Z;
      }

}
//-----------------------------------------------------------------------------
Value_P
Value::clone(const char * loc) const
{
Value_P ret(new Value(get_shape(), loc));

const Cell * src = &get_ravel(0);
Cell * dst = &ret->get_ravel(0);
const ShapeItem count = nz_element_count();

   loop(c, count)   dst++->init(*src++);

   ret->check_value(LOC);
   return ret;
}
//-----------------------------------------------------------------------------
/// lrp p.138: S←⍴⍴A + NOTCHAR (per column)
int32_t
Value::get_col_spacing(bool & not_char, ShapeItem col, bool framed) const
{
int32_t max_spacing = 0;
   not_char = false;

const ShapeItem ec = element_count();
const ShapeItem cols = get_last_shape_item();
const ShapeItem rows = ec/cols;
   loop(row, rows)
      {
        // compute spacing, which is the spacing required by this item.
        //
        int32_t spacing = 1;   // assume simple numeric
        const Cell & cell = get_ravel(col + row*cols);

        if (cell.is_pointer_cell())   // nested 
           {
             if (framed)
                {
                  not_char = true;
                  spacing = 1;
                }
             else
                {
                  Value_P v =  cell.get_pointer_value();
                  spacing = v->get_rank();
                  if (v->NOTCHAR())
                     {
                       not_char = true;
                       ++spacing;
                     }
                }
           }
        else if (cell.is_character_cell())   // simple char
           {
             spacing = 0;
           }
        else                                 // simple numeric
           {
             not_char = true;
           }

        if (max_spacing < spacing)   max_spacing = spacing;
      }

   return max_spacing;
}
//-----------------------------------------------------------------------------
int
Value::print_incomplete(ostream & out)
{
vector<Value *> incomplete;
bool goon = true;

   for (const DynamicObject * dob = all_values.get_prev();
        goon && (dob != &all_values); dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         goon = (dob != dob->get_prev());

         if (val->is_complete())   continue;

         out << "incomplete value at " << (const void *)val << endl;
         incomplete.push_back(val);

         if (!goon)
            {
              out << "Value::print_incomplete() : endless loop in "
                     "Value::all_values; stopping display." << endl;
            }
       }

   // then print more info...
   //
   loop(s, incomplete.size())
      {
        incomplete[s]->print_stale_info(out,
               (const DynamicObject *)
               incomplete[s]);
       }

   return incomplete.size();
}
//-----------------------------------------------------------------------------
int
Value::print_stale(ostream & out)
{
vector<Value *> stale_vals;
vector<const DynamicObject *> stale_dobs;
bool goon = true;
int count = 0;

   // first print addresses and remember stale values
   //
   for (const DynamicObject * dob = all_values.get_prev();
        goon && (dob != &all_values); dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         goon = (dob == dob->get_prev());

         if (val->owner_count)   continue;

         out << "stale value at " << (const void *)val << endl;
         stale_vals.push_back(val);
         stale_dobs.push_back(dob);

         if (!val->is_dirty())   ++count;

         if (!goon)
            {
              out << "Value::print_stale() : endless loop in "
                     "Value::all_values; stopping display." << endl;
            }
       }

   // then print more info...
   //
   loop(s, stale_vals.size())
      {
        const DynamicObject * dob = stale_dobs[s];
        Value * val = stale_vals[s];
        val->print_stale_info(out, dob);
       }

   // mark all dynamic values, and then unmark those known in the workspace
   //
   mark_all_dynamic_values();
   Workspace::unmark_all_values();

   // print all values that are still marked
   //
   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         Value * val = (Value *)dob;
         if (val->is_forever())   continue;

         // don't print values found in the previous round.
         //
         bool known_stale = false;
         loop(s, stale_vals.size())
            {
              if (val == stale_vals[s])
                 {
                   known_stale = true;
                   break;
                 }
            }
         if (known_stale)   continue;

         if (val->is_marked())
            {
              val->print_stale_info(out, dob);
              if (!val->is_dirty())   ++count;
              val->unmark();
            }
       }

   return count;
}
//-----------------------------------------------------------------------------
void
Value::print_stale_info(ostream & out, const DynamicObject * dob)
{
   out << "print_stale_info():   alloc(" << dob->where_allocated()
       << ") flags(" << get_flags() << ")" << endl;

   print_history(out, (const Value *)dob, LOC);

   try 
      {
        print_structure(out, 0, 0);
        Value_P Z = Quad_CR::fun.do_CR(7, *this);
        Z->print(out);
        out << endl;
      }
   catch (...)   { out << " *** corrupt ***"; }

   out << endl;
}
//-----------------------------------------------------------------------------
ostream &
operator<<(ostream & out, const Value & v)
{
   v.print(out);
   return out;
}
//-----------------------------------------------------------------------------
