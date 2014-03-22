#!/usr/bin/apl --script --

⍝ This is an APL CGI script that demonstrates the use of APL for CGI scripting
⍝ It outputs an HTML page like GNU APL's homepage at www.gnu.org.
⍝

⍝ Variable name conventions:
⍝
⍝ Variables starting with x, e.g. xB, are strings (simple vectors of
⍝ characters), i.e. 1≡ ≡xB and 1≡''⍴⍴⍴xB
⍝
⍝ Variables starting with y are vectors of character strings,
⍝ i.e. 2≡ ≡yB and 1≡''⍴⍴⍴yB
⍝
⍝ Certain characters in function names have the following meaning:
⍝
⍝ T - start tag
⍝ E - end tag
⍝ X - attributes

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

      ⍝ disable colored output and avoid APL line wrapping
      ⍝
      ]XTERM OFF
      ⎕PW←1000

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝
⍝ Document variables. Set them to '' so that they are always defined.
⍝ Override them in the document section (after )SAVE) as needed.
⍝
xTITLE←'<please-set-xTITLE>'
xDESCRIPTION←'<please-set-xDESCRIPTION>'

yBODY←0⍴'<please-set-yBODY>'

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ Misc. helper functions
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ append boxes A and B horizontally:
⍝
⍝ AABBB   AA      BBB
⍝ AABBB ← AA xbox BBB
⍝ AA      AA
⍝
∇yZ←yA xbox yB;LenA;H
 Assert 2 ≡ ≡yA ◊ Assert 1 ≡ ''⍴⍴⍴yA
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 LenA←⌈/⍴¨yA
 H←↑(⍴yA)⌈⍴yB
 yA←H↑LenA↑¨yA
 yZ←yA ,¨H↑yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ depth, rank, and shape of B
⍝ A: optional variable name
⍝ B: variable value
⍝
∇Z←A debug B;yPROP;yVAL
 yPROP←,⊂'<pre>At ',(,¯2 ⎕SI 3),': ' ◊ V←''
 →1+(0=⎕NC 'A')⍴⎕LC ◊  yPROP←'' ◊ V←A
 yPROP←yPROP,⊂'≡  ',V,': ',,⍕≡B
 yPROP←yPROP,⊂'⍴⍴ ',V,': ',,⍕⍴⍴B
 yPROP←yPROP,⊂'⍴  ',V,': ',,⍕⍴B
 yVAL←⊂[⎕IO+1]4 ⎕CR B
 ⊃yPROP xbox yVAL
 '</pre>'
 Z←B
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ check a condition
⍝
∇Assert B;COND;LOC;VAR
 →(1≡B)⍴0
 ' '
 COND←7↓,¯2 ⎕SI 4
 LOC←,¯2 ⎕SI 3
 '<pre>************************************************'
 ' '
 '*** Assertion (', COND, ') failed at ',LOC
 ''

 ⍝ show variable (assuming COND ends with the variable name)
 ⍝
 VCHAR←'∆⍙',⎕UCS ,64 96 ∘.+⍳26
 VAR←(⌽∧\⌽(COND∈VCHAR))/COND
 '</pre>'
 0 0⍴VAR debug ⍎VAR
 '<pre>'

 ⍝ show stack
 ⍝ 
 ' '
 'Stack:'
 7 ⎕CR ⊃¯1↓⎕SI 3
 ' '
 '************************************************</pre>'
 →
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ generic HTML related helper functions
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝


⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ emit a CGI header (if started from a web server)
∇xZ←HTTP_header
 xZ←'' ◊ →(0=⍴,⎕ENV "GATEWAY_INTERFACE")⍴0   ⍝ nothing if not run as CGI script
 ⍝
 ⍝ The 'Content-type: ...' string ends with LF (and no CR)
 ⍝ ⎕UCS 13 10 13 emits CR LF CR.
 ⍝ Together this gives two lines, each ending with CR/LF
 ⍝ Most tolerate LF without CR, but the CGI standard wants CR/LF
 ⍝
 xZ←'Content-type: text/html; charset=UTF-8', ⎕UCS 13 10 13
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ HTML attribute xA with value xB
⍝ xA="xB"
⍝
∇xZ←xA attr xB
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←' ',xA,'="',xB,'"'
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ Frequently used HTML attributes
⍝
0⍴⎕FX 'xZ←_in'           'xZ←"class"      attr "apl_in"'
0⍴⎕FX 'xZ←_out'          'xZ←"class"      attr "apl_out"'

0⍴⎕FX 'xZ←_alt  xB'      'xZ←"alt"        attr xB'
0⍴⎕FX 'xZ←_content xB'   'xZ←"content"    attr xB'
0⍴⎕FX 'xZ←_href xB'      'xZ←"href"       attr xB'
0⍴⎕FX 'xZ←_http_eq xB'   'xZ←"http-equiv" attr xB'
0⍴⎕FX 'xZ←_name xB'      'xZ←"name"       attr xB'
0⍴⎕FX 'xZ←_rel  xB'      'xZ←"rel"        attr xB'
0⍴⎕FX 'xZ←_src  xB'      'xZ←"src"        attr xB'
0⍴⎕FX 'xZ←_type xB'      'xZ←"type"       attr xB'
0⍴⎕FX 'xZ←_width B'      'xZ←"width"      attr ⍕B'
0⍴⎕FX 'xZ←_height B'     'xZ←"height"     attr ⍕B'
0⍴⎕FX 'xZ←_h_w B'        'xZ←(_height ↑B), _width 1↓B

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ HTML tag xA with attributes xX.
⍝ B determines the tag variant:
⍝
⍝   B = 1: HTML start Tag        <xA xX>
⍝   B = 2: HTML end Tag          </xA>
⍝   B = 3: XML start/end Tag     <xA xX/>
⍝
∇xZ←xA T[xX] B
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 xZ←'<',((B=2)⍴'/'),xA,xX,((B=3)⍴'/'),'>'
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ multi-line B tagged with A and /A
⍝ <A X>
⍝   B
⍝ </A>
⍝
∇yZ←xA TX_B_E[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←,⊂xA T[xX] 1
 yZ←yZ,indent yB
 yZ←yZ,⊂xA T 2
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ single-line xB tagged with xA and /xA
⍝ <xA xX> xB </xA>
⍝
∇yZ←xA TX_B_E_1[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂(xA T[xX] 1),xB,xA T 2
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ single-line xB tagged with xA (no /A)
⍝ <xA xX> xB
⍝
∇yZ←xA TX_B_1[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂(xA T[xX] 1),xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ convert single-line xB to multi-line yZ
⍝
∇yZ←x2y xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 yZ←,⊂xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ indent single-line xB
⍝
∇xZ←indent_2 xB
 xB←,xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←'  ',xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ indent multi-line xB
⍝
∇yZ←indent yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←indent_2 ¨,yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ disclose one-line yB and print it
⍝
∇emit_1 yB
 ⊃yB
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ disclose multi-line yB and print it
⍝
∇emit yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 emit_1¨yB
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ specific HTML Elements
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag xB as ANCHOR with URI xHREF
⍝ <A href=xHREF> xB </A>
⍝
∇xZ←xHREF A xB
 Assert 1 ≡ ≡xB    ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xHREF ◊ Assert 1 ≡ ''⍴⍴⍴xHREF
 xZ←,⊃"A" TX_B_E_1[_href xHREF] xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag xB as TITLE
⍝ <TITLE> xB </TITLE>
⍝
∇yZ←Title xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 yZ←'TITLE' TX_B_E_1 xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag xB as IMAGE
⍝ <IMG xB>
⍝
∇yZ←Img[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂'IMG' T[xX] 1
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag xB as LINK
⍝ <LINK xB>                         ⍝
⍝
∇yZ←Link xX
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂'LINK' T[xX] 1
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag yB as STYLE
⍝ <STYLE xX >
⍝   yB
⍝ </STYLE>
⍝
∇yZ←Style[xX] yB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'STYLE' TX_B_E[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag xB as LIST ITEM
⍝ <LI> xB
⍝
∇xZ←Li xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←'<LI> ', xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag yB as ORDERED LIST
⍝ <OL>
⍝   <LI> B[1]
⍝   <LI> B[2]
⍝   ...
⍝ </OL>
⍝
∇yZ←Ol[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 yZ←'OL' TX_B_E[xX] Li¨yB
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag yB as UNORDERED LIST
⍝ <UL>
⍝   <LI> B[1]
⍝   <LI> B[2]
⍝   ...
⍝ </UL>
⍝
∇yZ←Ul[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 yZ←'UL' TX_B_E[xX] Li¨yB
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ Wrappers for frequently used HTML elements
⍝
0⍴⎕FX 'yZ←H1[xX] xB'   'yZ←"H1" TX_B_E_1[xX] xB'
0⍴⎕FX 'yZ←H2[xX] xB'   'yZ←"H2" TX_B_E_1[xX] xB'
0⍴⎕FX 'yZ←H3[xX] xB'   'yZ←"H3" TX_B_E_1[xX] xB'
0⍴⎕FX 'yZ←H4[xX] xB'   'yZ←"H4" TX_B_E_1[xX] xB'

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ return HEAD using xTITLE
⍝ <HEAD>
⍝   B
⍝ </HEAD>
⍝
∇yZ←Head;yB
 yB←Title xTITLE
 yB←yB,,⊂'META' T[(_http_eq 'Content-Type'), _content 'text/html; charset=UTF-8'] 1
 yB←yB,,⊂'META' T[(_name 'description'), _content xDESCRIPTION] 1
 yB←yB,Link (_rel  'stylesheet'),(_type 'text/css'), _href 'apl-home.css'
 yB←yB,Style[_type  'text/css'] ,⊂''

 yZ←'HEAD' TX_B_E yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ tag yB as BODY
⍝ <BODY X>
⍝   B
⍝ </BODY>
⍝
∇yZ←Body[xX] yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'BODY' TX_B_E[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ HTML document with body yB
⍝ <HTML X>
⍝   B
⍝ </HTML>
⍝
∇yZ←Html[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'HTML' TX_B_E Head, Body[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ The entire document
⍝
∇yZ←Document 
 yZ←    x2y HTTP_header
 yZ←yZ, x2y '<!DOCTYPE html>'
 yZ←yZ, Html yBODY
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

      ⍝ save the functions and variables defined so far, You can do that
      ⍝ to create a workspace containing the functions and variables defined 
      ⍝ above that you can )COPY into other web pages.
      ⍝
      ⍝ )WSID APL_CGI
      ⍝ )SAVE

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ The content of the HTML page
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ Return xHTTP_GNU or xHTTP_JSA
⍝ depending on the CGI variable SERVER_NAME
⍝
∇xZ←Home;xS
 xS←⊃(⎕⎕ENV 'SERVER_NAME')[;⎕IO + 1]
 xZ←"192.168.0.110/apl"    ⍝ Jürgen's home ?
 →(S≡'192.168.0.110')/0    ⍝ yes, this script was called by apache
 →(S≡'')/0                 ⍝ yes, this script called directly
 xZ←xHTTP_GNU,'/apl'       ⍝ no
∇

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ set xTITLE and xDESCRIPTION that go into the HEAD section of the page
⍝
xTITLE←'GNU APL'
xDESCRIPTION←'Welcome to GNU APL'

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ some URIs used in the BODY
⍝
xHTTP_GNU←"http://www.gnu.org/"
xHTTP_JSA←"http://192.168.0.110/apl/"
xFTP_GNU←"ftp://ftp.gnu.org"
xFTP_APL←xFTP_GNU,"/gnu/apl"
xCYGWIN←"www.cygwin.org"
xMIRRORS←'http://www.gnu.org/prep/ftp.html'
xGNU_PIC←_src xHTTP_GNU, "graphics/gnu-head-sm.jpg"

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ some file names used in the BODY
⍝
xAPL_VERSION←'apl-1.1'
xTARFILE←xAPL_VERSION,  '.tar.gz'
xRPMFILE←xAPL_VERSION,  '-0.i386.rpm'
xSRPMFILE←xAPL_VERSION, '-0.src.rpm'
xDEBFILE←xAPL_VERSION,  '-1_i386.deb'
xSDEBFILE←xAPL_VERSION, '-1.debian.tar.gz'
xAPL_TAR←xFTP_GNU, '/', xTARFILE
xMAIL_GNU←'gnu@gnu.org'
xMAIL_WEB←'bug-apl@gnu.org'
xMAIL_APL←'bug-apl@gnu.org'
xSVN_APL←'https://savannah.gnu.org/svn/?group=apl'

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ some features of GNU APL
⍝
yFEATURES←           ⊂ 'nested arrays and related functions'
yFEATURES←yFEATURES, ⊂ 'complex numbers, and'
yFEATURES←yFEATURES, ⊂ 'a shared variable interface'
yFEATURES←Ul yFEATURES

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ Installation instructios
⍝
∇yZ←INSTALL;I1;I2;I3;I4
I1←      'Visit one of the ', xMIRRORS A 'GNU mirrors'
I1←  I1, ' and download the tar file <B>', xTARFILE,'</B> in directory'
I1←⊂ I1, ' <B>apl</B>.'
I2←⊂     'Unpack the tar file: <B>tar xzf ', xTARFILE, '</B>'
I3←⊂     'Change to the newly created directory: <B>cd ', xAPL_VERSION, '</B>'
I4←      'Read (and follow) the instructions in files <B>INSTALL</B>'
I4←⊂ I4, ' and <B>README-*</B>'
yZ←⊃ Ol I1, I2, I3, I4
∇

      ⍝ ⎕INP acts like a HERE document in bash. The monadic form ⎕INP B
      ⍝ reads subsequent lines from the input (i.e. the lines below ⎕INP
      ⍝ if ⎕INP is called in a script) until pattern B is seen. The lines
      ⍝ read are then returned as the result of ⎕INP.
      ⍝
      ⍝ The dyadic form A ⎕INP B acts like the monadic form ⎕INP B.
      ⍝ A is either a single string or a nested value of two strings.
      ⍝
      ⍝ Let A1←A2←A if A is a string or else A1←A[1] and A2←A[2] if A is
      ⍝ a nested 2-element vector containing two strings.
      ⍝
      ⍝ Then every pattern A1 expression A2 is replaced by ⍎ expression.
      ⍝
      ⍝ We first give an example of ⎕INP in the style of PHP and another,
      ⍝ more compact, example further down below.
      ⍝
      yBODY← '<?apl' '?>' ⎕INP 'END-OF-⎕INP'   ⍝ php style

<DIV class="c1">
<?apl H1[''] xTITLE ?>
<TABLE>
  <TR>
    <TD> <?apl Img xGNU_PIC, (_alt 'Astrid'), _h_w 122 129 ?>
    <TD width="20%">
    <TD><I> Rho, rho, rho of X<BR>
         Always equals 1<BR>
         Rho is dimension, rho rho rank.<BR>
         APL is fun!</I><BR>
         <BR>
         <B>Richard M. Stallman</B>, 1969<BR>
  </TR>
</TABLE>

<BR><BR><BR>
</DIV>
<DIV class="c2">
<B>GNU APL</B> is a free interpreter for the programming language APL.
<BR><BR>
The APL interpreter is an (almost) complete implementation of
<I><B>ISO standard 13751</B></I> aka.
<I><B>Programming Language APL, Extended.</B></I>
<BR>
<BR>
The APL interpreter has implemented:
<?apl ⊃ yFEATURES ?>

In addition, <B>GNU APL</B> can be scripted. For example,
<?apl x2y 'APL_demo.html' A "<B>this HTML page</B>" ?>
is the output of a CGI script written in APL.
<BR>
<BR>
GNU APL was written and is being maintained by Jürgen Sauermann.
</DIV>
<DIV class="c3">

<?apl H2[''] 'Downloading and Installing GNU APL' ?>
GNU APL should be available on every 
<?apl  xMIRRORS A 'GNU mirror' ?>
(in directory <B>apl</B>) and at
<?apl  xFTP_APL A xFTP_GNU ?>.

<?apl H4[''] 'Normal Installation of GNU APL' ?>
The normal (and fully supported) way to install GNU APL is this:

<?apl ⊃ INSTALL ?>

<?apl H4[''] 'GNU APL for WINDOWs' ?>

GNU APL compiles under CYGWIN, (see
<?apl  ('http://',xCYGWIN) A xCYGWIN ?>),
provided that the necessary libraries are installed. A 32-bit <B>apl.exe</B>
that may run under CYGWIN lives in the download area. Use at your own risk and
see <B>README-5-WINDOWS</B> for further information.

<?apl H4[''] 'Subversion (SVN) repository for GNU APL' ?>

You can also check out the latest version of GNU APL from its subversion
repository on Savannah:
<BR>
<BR>
<B>svn co http://svn.savannah.gnu.org/svn/apl/trunk</B>.
<BR>
<BR>
Here is <?apl x2y xSVN_APL A "<EM>more information</EM>" ?>
about using Subversion with GNU APL.

<?apl H4[''] 'RPMs for GNU APL' ?>

For RPM based GNU/Linux distributions we have created source and binary RPMs.
Look for files <B><?apl xRPMFILE ?></B> (binary RPM for i386) or 
<B><?apl xSRPMFILE ?></B> (source RPM). If you encounter a problem with these
RPMs, then please report it, but with a solution, since the maintainer of
GNU APL may use a GNU/Linux distribution with a different package manager.

<?apl H4[''] 'Debian packages for GNU APL' ?>

For Debian based GNU/Linux distributions we have created source and binary 
packages for Debian. Look for files <B><?apl xDEBFILE ?></B> (binary Debian
package for i386) or <B><?apl xSDEBFILE ?></B> (Debian source package).
If you encounter a problem with these packages, then please report it,
but with a solution, since the maintainer of GNU APL may use a GNU/Linux
distribution with a different package manager.

<?apl H4[''] 'GNU APL Binary' ?>

If you just want to quickly give GNU APL a try, and if you are very lucky
(which includes having shared libraries libreadline.so.5 and liblapack.so.3gf
installed on your machine) then you may be able to start the compiled
GNU APL binary <B>apl</B> in the directory <B>apl</B> rather than 
installing the entire packet . The binary MAY run on a 32-bit i686 Ubuntu.
Chances are, however, that it does NOT work, Please DO NOT report any
problems if the binary does not run on your machine. Instead use the standard
installation method above.
<BR><BR>
<B>Note:</B> The program <B>APnnn</B> (a support program for shared variables)
is not provided in binary form, so you should start the <B>apl</B> binary with
command line option --noSV. Note as well that the binary <B>apl</B> will not
be updated with every GNU APL release. Therefore it will contain errors that
have been corrected already.
</DIV>
<DIV class="c4">
<?apl H2[''] 'Reporting Bugs' ?>

GNU APL is made up of about 50,000 lines of C++ code. In a code of that
size, programming mistakes are inevitable. Even though mistakes are hardly
avoidable, they can be <B>corrected</B> once they are found. In order to
improve the quality of GNU APL, we would like to encourage you to report
errors that you find in GNU APL to
<?apl x2y ("mailto:", xMAIL_APL) A "<EM>", xMAIL_APL, "</EM>" ?>.
<BR><BR>
Your email should include a small example of how to reproduce the fault.
</DIV><DIV class="c5">
<?apl H2[''] 'Documentation' ?>
We have an <?apl x2y 'apl.html' A "<B>info manual</B>" ?> for GNU APL.

We are also looking for <B>free</B> documentation on APL in general
(volunteers welcome) that can be published here. A "Quick start" document
for APL is planned but the work has not started yet.
<BR><BR>
The C++ source files for GNU APL are Doxygen documented. You can generate
this documentation by running <B>make DOXY</B> in the top level directory
of the GNU APL package.
</DIV>
END-OF-⎕INP


      ⍝ the text above used an 'escape style' similar to PHP
      ⍝ (using <?apl ... ?> instead of <?php ... ?>). This style also
      ⍝ resembles the tagging of HTML.
      ⍝
      ⍝ By calling ⎕INP with different left arguments you can use your
      ⍝ preferred style, for example the more compact { ... } style
      ⍝ as shown in the following example:
      ⍝
      yBODY←yBODY, (,¨'{}') ⎕INP 'END-OF-⎕INP'   ⍝ more compact style
<DIV class="c6">
Return to {x2y "http://www.gnu.org/home.html" A "GNU's home page"}.
<P>

Please send FSF &amp; GNU inquiries &amp; questions to

{x2y ("mailto:", xMAIL_GNU) A "<EM>", xMAIL_GNU, "</EM>"}.
There are also
{x2y "http://www.gnu.org/home.html#ContactInfo" A "other ways to contact"}
the FSF.
<P>
Please send comments on these web pages to
{x2y ("mailto:", xMAIL_WEB) A "<EM>", xMAIL_WEB, "</EM>"}.
send other questions to
{x2y ("mailto:", xMAIL_GNU) A "<EM>", xMAIL_GNU, "</EM>"}.
<P>
Copyright (C) 2014 Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110,  USA
<P>
Verbatim copying and distribution of this entire article is
permitted in any medium, provided this notice is preserved.<P>
</DIV>
END-OF-⎕INP

      emit Document

      '<!--'
      )VARS
      )SI
      '-->'
      )OFF
