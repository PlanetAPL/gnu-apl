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

#include "Error.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
PointerCell::PointerCell(Value_P val)
{
   new (&value._valp()) Value_P(val);
}
//-----------------------------------------------------------------------------
bool
PointerCell::equal(const Cell & other, APL_Float qct) const
{
   if (!other.is_pointer_cell())   return false;

Value_P A = get_pointer_value();
Value_P B = other.get_pointer_value();

   if (!A->same_shape(B))                 return false;

const uint64_t count = A->element_count();
   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
          return false;

   return true;
}
//-----------------------------------------------------------------------------
bool
PointerCell::greater(const Cell * other, bool ascending) const
{
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
void
PointerCell::release(const char * loc)
{
   ptr_clear(value._valp(), loc);
   new (this) Cell;
}
//-----------------------------------------------------------------------------
Value_P
PointerCell::get_pointer_value() const
{
Value_P ret = value._valp();
   return ret;
}
//-----------------------------------------------------------------------------
CellType
PointerCell::compute_cell_types() const
{
   return CellType(CT_POINTER | get_pointer_value()->compute_cell_types());
}
//-----------------------------------------------------------------------------
PrintBuffer
PointerCell::character_representation(const PrintContext & pctx) const
{
Value_P val = get_pointer_value();
   Assert(val);

   if (pctx.get_style() == PR_APL_FUN)   // APL function display
      {
        const ShapeItem ec = val->element_count();
        UCS_string ucs;

        if (val->is_char_vector())
           {
             ucs.append(UNI_SINGLE_QUOTE);
             loop(e, ec)
                {
                  const Unicode uni = val->get_ravel(e).get_char_value();
                  ucs.append(uni);
                  if (uni == UNI_SINGLE_QUOTE)   ucs.append(uni);   // ' -> ''
                }
             ucs.append(UNI_SINGLE_QUOTE);
           }
        else
           {
             ucs.append(UNI_ASCII_L_PARENT);
             loop(e, ec)
                {
                  PrintBuffer pb = val->get_ravel(e).
                        character_representation(pctx);
                  ucs.append(UCS_string(pb, 0));

                  if (e < ec - 1)   ucs.append(UNI_ASCII_SPACE);
                }
             ucs.append(UNI_ASCII_R_PARENT);
           }

        ColInfo ci;
        return PrintBuffer(ucs, ci);
      }

PrintBuffer ret(*val, pctx);
   ret.get_info().flags &= ~CT_MASK;
   ret.get_info().flags |= CT_POINTER;

   if (pctx.get_style() & PST_CS_INNER)
      {
        const ShapeItem ec = val->element_count();
        if (ec == 0)   // empty value
           {
             int style = pctx.get_style();
             loop(r, val->get_rank())
                {
                  if (val->get_shape_item(r) == 0)   // an empty axis
                     {
                       if (r == val->get_rank() - 1)   style |= PST_EMPTY_LAST;
                       else                            style |= PST_EMPTY_NLAST;
                     }
                }

             // do A←(1⌈⍴A)⍴⊂↑A
             Value_P proto = val->prototype(LOC);

             // compute a shape like ⍴ val but 0 replaced with 1.
             //
             Shape sh(val->get_shape());
             loop(r, sh.get_rank())
                 {
                   if (sh.get_shape_item(r) == 0)   sh.set_shape_item(r, 1);
                 }

             if (sh.get_volume() == 1)   // one prototype
                {
                  ret = PrintBuffer(*proto, pctx);
                  ret.add_frame(PrintStyle(style), proto->get_rank(),
                                proto->compute_depth());
                }
             else                           // several prototypes
                {
                  Value_P proto_reshaped(new Value(sh, LOC));
                  Cell * c = &proto_reshaped->get_ravel(0);
                  const ShapeItem len = proto_reshaped->element_count();

                  // store proto in the first ravel item, and copies of proto in
                  // the subsequent ravel items
                  //
                  new (c++) PointerCell(proto);
                  loop(rv, len - 1)   new (c++) PointerCell(proto->clone(LOC));

                  ret = PrintBuffer(*proto_reshaped, pctx);
                  ret.add_frame(PrintStyle(style), proto_reshaped->get_rank(),
                                proto_reshaped->compute_depth());
                }
           }
       else
           {
             ret.add_frame(pctx.get_style(), val->get_rank(),
                           val->compute_depth());
           }
      }

   ret.get_info().int_len  = ret.get_width(0);
   ret.get_info().real_len = ret.get_width(0);
   return ret;
}
//-----------------------------------------------------------------------------

