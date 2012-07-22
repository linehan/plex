#ifndef __TOKEN_H
#define __TOKEN_H

/****************************************************************************
 * TOKENS
 ****************************************************************************/

enum token {
        EOS = 1,      // end of string
        ANY,          // .
        AT_BOL,       // ^ 
        AT_EOL,       // $
        CCL_END,      // ]
        CCL_START,    // [
        CLOSE_CURLY,  // }
        CLOSE_PAREN,  // )
        CLOSURE,      // *
        DASH,         // -
        END_OF_INPUT, // EOF
        L,            // literal character
        OPEN_CURLY,   // {
        OPEN_PAREN,   // (
        OPTIONAL,     // ?
        OR,           // |
        PLUS_CLOSE    // +
};


enum token TOKEN_MAP[] = {
//  ^@  ^A  ^B  ^C  ^D  ^E  ^F  ^G  ^H  ^I  ^J  ^K  ^L  ^M  ^N	
     L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  ^O  ^P  ^Q  ^R  ^S  ^T  ^U  ^V  ^W  ^X  ^Y  ^Z  ^[  ^\  ^]	
     L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  ^^  ^_  SPACE  !   "   #    $        %   &    '		
    L,  L,  L,     L,  L,  L,   AT_EOL,  L,  L,   L,
//  (		 )            *	       +           ,  -     .   
    OPEN_PAREN,  CLOSE_PAREN, CLOSURE, PLUS_CLOSE, L, DASH, ANY,
//  /   0   1   2   3   4   5   6   7   8   9   :   ;   <   =	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  >         ?							
    L,        OPTIONAL,
//  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  O   P   Q   R   S   T   U   V   W   X   Y   Z   		
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  [		\	]		^			
    CCL_START,	L,	CCL_END, 	AT_BOL,
//  _   `   a   b   c   d   e   f   g   h   i   j   k   l   m	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  n   o   p   q   r   s   t   u   v   w   x   y   z   	
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
//  {            |    }            DEL 				
    OPEN_CURLY,  OR,  CLOSE_CURLY, L
};


#endif

