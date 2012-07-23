#ifndef _MACRO_H
#define _MACRO_H

#define MAC_NAME_MAX 34 // Max macro name length
#define MAC_TEXT_MAX 80	// Max macro text length 


struct macro_t {
        char name[MAC_NAME_MAX];
        char text[MAC_TEXT_MAX];
};


void new_macro(char *def);
char *get_macro(char **namep);


#endif
