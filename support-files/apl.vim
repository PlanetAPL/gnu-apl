" Vim syntax file
" Language:     APL
" Maintainer:   Jürgen Sauermann <bug-apl@gnu.org>
" Last Change:  2012 Mar 3

" standard APL comment
syn region aplComment     start="⍝" end="$"

" non-standard APL comment to make APL scriptable
syn region aplComment     start="#" end="$"

" standard APL string
syn region aplString     oneline start="'" end="$"
syn region aplString     oneline start='"' end="$"
syn region aplString     oneline start="'" end="'"
syn region aplString     oneline start='"' end='"'

" Numbers...
"                         +----- optional mantissa sign          +-- J
"                         |  +-- mantissa  --+ +-- exponent --+  |   (optional)
"                         |  |               | |   (optional) |  |
"                         |  | a) .nnn       | |              |  |
"                         |  | b) nnn.mmm    | | +- expo sign |  |
"                         |  |               | | |  (opt.)    |  |
"                         |  |               | | |            |  |
syn match aplNumber      "¯\=\.[0-9]\+\(E¯\=[0-9]\+\)\=J\="
syn match aplNumber      "¯\=[0-9]\+\(\.[0-9]*\)\=\(E¯\=[0-9]\+\)\=J\="

" user defined and distinguished names
syn match aplIdentifier	 "[A-Z_∆⍙⎕][0-9A-Z_∆⍙]*"
syn match aplIdentifier	 "⍞"

syn match aplStatement   "[∇⍫◊]"
syn match aplStatement	 "^ *[A-Z_∆⍙⎕][0-9A-Z_∆⍙]*:"
syn match aplStatement	 "^ *\[[0-9.]*\]"

hi def link aplComment       CursorColumn
hi def link aplString        LineNr
hi def link aplNumber        Number
hi def link aplIdentifier    Identifier
hi def link aplStatement     Statement

