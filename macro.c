#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lib/textutils.h"
#include "lib/map.h"
#include "macro.h"
#include "nfa.h"


struct map_t *MACROTABLE; /* Symbol table for macro definitions */


/**
 * new_macro
 * ````````` 
 * Add a new macro to the table. 
 * If two macros have the same name, the second one takes precedence. 
 * A definition takes the form:
 *      
 *              name <whitespace> text [<whitespace>]
 * 
 * Whitespace at the end of the line is ignored.
 */
void new_macro(char *def)
{
        static int first = 1;
        char  *name; // Name part of macro	
        char  *text; // Text part of macro
        char  *edef; // Pointer to the end of text part
        struct macro_t *p;

        if (first) {
	        first = 0;
	        MACROTABLE = new_map(31);
        }

        /* Isolate name */
        for (name = def; *def && !isspace(*def); def++)	   
                ;

        if (*def)
                *def++ = '\0' ;

        /* 
         * Isolate the definition text. 
         * Because you need to discard any trailing whitespace on the 
         * line, this is a bit tricky. The first while loop skips the 
         * preceding whitespace. The for loop is looking for end of string. 
         * If you find a ws character (and the \n at the end of string is ws), 
         * remember the position as a potential end of string.
         */
        while (isspace(*def))
                ++def;

        text = def;
        edef = NULL;

        while (*def) {
                if (!isspace(*def)) ++def;
                else {
                        for (edef = def++; isspace(*def); ++def)
                                ;
                }
        }

        if (edef)
	        *edef = '\0';
	
        /* Add the macro to the symbol table  */
        p  = (struct macro_t *)new_symbol(sizeof(struct macro_t));
        slcpy(p->name, name, MAC_NAME_MAX);
        slcpy(p->text, text, MAC_TEXT_MAX);
        add_symbol(MACROTABLE, p);
}


/** 
 * get_macro
 * `````````
 * Get the contents of a macro having the indicated name.
 * 
 * @namep: Pointer to the name of the macro.
 * Return: NUL-terminated string with the macro definition. 
 *
 * NOTES
 * Aborts with a message if no macro matching the @namep exists.
 * The macro name includes the brackets.
 * @namep is modified to point past the '}'
 */
char *get_macro(char **namep)
{
        struct macro_t *mac;
        char *p;
					
        /* Hit a '{', skip it and find '}'. */
        if (!(p = strchr(++(*namep), '}')))
	        halt(SIGABRT, "Bad macro.");		
        else {
                *p = '\0'; // Overwrite the '}'

	        if (!(mac = (struct macro_t *)get_symbol(MACROTABLE, *namep)))
                        halt(SIGABRT, "No macro.");

	        *p++ = '}'; // Re-write the '}'
                *namep = p; // Update name pointer

	        return mac->text;
        }
        return "ERROR";	// Shouldn't happen.
}


