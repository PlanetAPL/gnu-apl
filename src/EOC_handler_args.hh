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

#ifndef __EOC_HANDLER_ARGS_HH_DEFINED_
#define __EOC_HANDLER_ARGS_HH_DEFINED_

#include "Common.hh"
#include "Shape.hh"
#include "Value.hh"

/// arguments of the EOC handler for ⎕EA
struct quad_EA
{
  // the EOC handler for ⎕EC has no additional arguments
};

/// arguments of the EOC handler for ⎕EC
struct quad_EC
{
  // the EOC handler for ⎕EC has no arguments
};

/// arguments of the EOC handler for ⎕INP
struct quad_INP
{
  UCS_string_list * lines;       /// the lines of the final result.

  UCS_string * end_marker;       ///< the end of the data read by ⎕INP
  UCS_string * esc1;             ///< start marker (escape from ⎕INP)
  UCS_string * esc2;             ///< end marker (return to ⎕INP)

  bool done;                     ///< true if end_marker read
};

/// arguments of the EOC handler for A ∘.f B
struct OUTER_PROD
{
  Function * RO;     ///< user defined function
  ShapeItem a;       ///< current A index
  ShapeItem len_A;   ///< number of cells in left arg
  ShapeItem b;       ///< current B index
  ShapeItem len_B;   ///< number of cells in right arg
  const Cell * cA;   ///< current left arg
  const Cell * cB;   ///< current right arg
  int how;           ///< how to continue in finish_outer_product()
};

/// arguments of the EOC handler for A g.f B
struct INNER_PROD
{
  Value_P * args_A;    ///< left args of RO
  ShapeItem a;         ///< current A1 index
  ShapeItem items_A;   ///< number of cells in A1
  Function * LO;       ///< left user defined function
  Function * RO;       ///< right user defined function
  Value_P * args_B;    ///< right args of RO
  ShapeItem b;         ///< current B1 index
  ShapeItem items_B;   ///< number of cells in B1
  ShapeItem v1;        ///< current LO index
  int how;             ///< how to continue in finish_inner_product()
  bool last_ufun;      ///< true for the last user defined function call
};

/// arguments of the EOC handler for one f/B result cell
struct REDUCTION
{
   /// initialize \b this REDUCTION object
   void init(const Shape3 & Z3, Function * LO,
             const Cell * cB, ShapeItem bm, ShapeItem A0, int A0_inc)
      {
        need_pop = false;
        
        frame.frame_init(Z3, cB, bm, A0_inc);
        beam.beam_init(LO, cB, Z3.l(), A0);
      }

   /// true if SI was pushed
   bool need_pop;

   /// information about one beam
   struct _beam
      {
         /// initialize \b this _beam (first beam in frame)
         void beam_init(Function * _LO, const Cell * B_h0l,
                   ShapeItem _dist, ShapeItem _A0)
            { LO = _LO;
              if (_A0 < 0)   { dist = _dist;     length = - _A0; }
              else           { dist = - _dist;   length = _A0;   }

              reset(B_h0l);
            }

         /// reset \b this _beam (next beam in frame)
         void reset(const Cell * B_h0l)
            { idx = 0;
              if (dist < 0)    src = B_h0l - (length * dist);
              else             src = B_h0l - dist;
            }

         /// advance B in \b this beam
         const Cell * next_B()
            { ++idx;  src += dist;  return src; }

         /// return true iff this beam is finished (one reduction result item
         /// has been computed
         bool done() const
            { return idx >= length; }

         ShapeItem dist;     ///< the distance between beam elements
         ShapeItem length;   ///< number of beam elements
         Function * LO;      ///< left operand

         const Cell * src;   ///< the next cell of the beam
         ShapeItem    idx;   ///< the current index of base
      } beam;   ///< the current beam

   /// information about all beams
   struct _frame
      {
        /// initialize \b this _frame
        void frame_init(const Shape3 & Z3, const Cell *  _cB,
                        ShapeItem _max_bm, int _A0_inc)
           {
             cB = _cB;

             max_h = Z3.h();
             max_bm = _max_bm;
             max_zm = Z3.m();
             max_l = Z3.l();
             A0_inc = _A0_inc;

             h = 0;
             m = 0;
             l = 0;
           }

        /// advance Z in \b this frame
       void next_hml()
           { ++l;
             if (l >= max_l)    { l = 0;   ++m; }
             if (m >= max_zm)   { m = 0;   ++h; }
           }

        /// return true iff this frame is finished (the last reduction result
        /// item has been computed
        bool done() const
           { return h >= max_h; }

        /// the beam start for Z[h;m]
        const Cell * beam_start()
           { return cB + l + max_l*(m * (1 - A0_inc) + max_bm * h); } 

        // fixed variables
        ShapeItem    max_bm;       ///< max. m for B
        ShapeItem    max_h;        ///< max. h
        ShapeItem    max_zm;       ///< max. m for Z
        ShapeItem    max_l;        ///< max. l
        int          A0_inc;       ///< window increment (= 1 for scan)

        // running variables
        const Cell * cB;           ///< first ravel item in B
        ShapeItem    h;            ///< current h
        ShapeItem    m;            ///< current m
        ShapeItem    l;            ///< current l
      } frame;   ///< the current frame
};

/// arguments of the EOC handler for A f¨ B
struct EACH_ALB
{
  const Cell * cA;   ///< current left arg
  uint32_t dA;       ///< cA increment (0 or 1)
  Function * LO;     ///< user defined function
  const Cell * cB;   ///< current left arg
  uint32_t dB;       ///< cB increment (0 or 1)
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
  int how;           ///< how to continue in finish_eval_LB()
};

/// arguments of the EOC handler for f¨ B
struct EACH_LB
{
  Function * LO;     ///< user defined function
  const Cell * cB;   ///< current left arg
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
  int how;           ///< how to continue in finish_eval_LB()
};

/// arguments of the EOC handler for f⍤[X] B
struct RANK_LXB
{
  Function * LO;                         ///< user defined function
  ShapeItem h;                           ///< current high index
  char _sh_B_low [ sizeof(Shape) ];      ///< low dimensions of B
  char _sh_B_high[ sizeof(Shape) ];      ///< high dimensions of B
  ShapeItem ec_B_low;                    ///< items in _sh_B_low
  const Cell * cB;                       ///< current B cell
  Value_P * ZZ ;                         ///< current ZZ value
  char _sh_Z_max_low[ sizeof(Shape) ];   ///< max. low dimensions of Z
  ShapeItem ec_high;                     ///< max. high index
  int how;                               ///< how to finish_eval_LXB()

  /// return sh_B_low
  Shape & get_sh_B_low()       { return *(Shape *)&_sh_B_low;     }

  /// return sh_B_high
  Shape & get_sh_B_high()      { return *(Shape *)&_sh_B_high;    }

  /// return sh_Z_max_low
  Shape & get_sh_Z_max_low()   { return *(Shape *)&_sh_Z_max_low; }
};

/// arguments of the EOC handler for A f⍤[X] B
struct RANK_ALXB
{
  Function * LO;                         ///< user defined function
  ShapeItem h;                           ///< current high index
  bool repeat_A;                         ///< skalar-extend A
  char _sh_A_low [ sizeof(Shape) ];      ///< low dimensions of A
  ShapeItem ec_A_low;                    ///< items in _sh_B_low
  const Cell * cA;                       ///< current A cell
  bool repeat_B;                         ///< skalar-extend B
  char _sh_B_low [ sizeof(Shape) ];      ///< low dimensions of B
  char _sh_B_high[ sizeof(Shape) ];      ///< high dimensions of B
  ShapeItem ec_B_low;                    ///< items in _sh_B_low
  const Cell * cB;                       ///< current B cell
  Value_P * ZZ;                          ///< current ZZ value
  char _sh_Z_max_low[sizeof(Shape)];     ///< max. low dimensions of Z
  ShapeItem ec_high;                     ///< max. high index
  int how;                               ///< how to finish_eval_LXB()

  /// return sh_A_low
  Shape & get_sh_A_low()       { return *(Shape *)&_sh_A_low;     }

  /// return sh_B_low
  Shape & get_sh_B_low()       { return *(Shape *)&_sh_B_low;     }

  /// return sh_B_high
  Shape & get_sh_B_high()      { return *(Shape *)&_sh_B_high;    }

  /// return sh_Z_max_low
  Shape & get_sh_Z_max_low()   { return *(Shape *)&_sh_Z_max_low; }
};

/// the second argument for an EOC_HANDLER. The actual type depends on the
/// handler. An EOC_arg contains all information that is necessary for
/// the EOC handler to compute the result token.
class EOC_arg
{
public:
   /// constructor for dyadic derived function
   EOC_arg(Value_P vpZ, Value_P vpB, Value_P vpA)
   : Z(vpZ),
     B(vpB),
     A(vpA)
   {}

   /// constructor for monadic derived function without result (or
   /// result created at return)
   EOC_arg(Value_P vpB)
   : B(vpB)
   {}

   /// constructor for monadic derived function with result
   EOC_arg(Value_P vpZ, Value_P vpB)
   : Z(vpZ),
     B(vpB)
   {}

   /// copy constructor
   EOC_arg(const EOC_arg & other)
   : Z(other.Z),
     B(other.B),
     A(other.A),
     V1(other.V1),
     V2(other.V2),
     RO_A(other.RO_A),
     RO_B(other.RO_B)
   { u = other.u; }

   /// result
   Value_P Z;

   /// left argument
   Value_P B;

   /// right argument
   Value_P A;

  /// INNER_PROD: argument for LO-reduction
  /// OUTER_PROD: helper value for non-pointer left RO argument
  Value_P V1;

  /// INNER_PROD: accumulator for LO-reduction
  /// OUTER_PROD: helper value for non-pointer right RO argument
  Value_P V2;

  /// OUTER_PROD: left RO argument
  Value_P RO_A;

   /// OUTER_PROD: right RO argument
  Value_P RO_B;

   /// additional EOC handler specific arguments
   union EOC_arg_u
      {
        quad_EA    u_quad_EA;
        quad_EC    u_quad_EC;
        quad_INP   u_quad_INP;
        OUTER_PROD u_OUTER_PROD;
        INNER_PROD u_INNER_PROD;
        REDUCTION  u_REDUCTION;
        EACH_ALB   u_EACH_ALB;
        EACH_LB    u_EACH_LB;
        RANK_LXB   u_RANK_LXB;
        RANK_ALXB  u_RANK_ALXB;
      } u;
};

/// the type of a function to be called at the end of a context.
/// the function returns true to retry and false to continue with token.
typedef bool (*EOC_HANDLER)(Token & token, EOC_arg & arg);

//-----------------------------------------------------------------------------

#endif // __EOC_HANDLER_ARGS_HH_DEFINED__
