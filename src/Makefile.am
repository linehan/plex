AUTOMAKE_OPTIONS = foreign subdir-objects
#                      |            \
#                  non-GNU        compile objects in the 
#                                 indicated relative paths
# optimize warnings debug info
# lvl 3 \     /    /  
CFLAGS=-O3 -Wall -g -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\""
#             
#             
#             

bin_PROGRAMS  = plex 

plex_SOURCES = main.c          \
               lib/file.c      \
               lib/set.c       \
               lib/textutils.c \
               lib/debug.c     \
               input.c         \
               scan.c          \
               lex.c           \
               macro.c         \
               nfa.c           \
               dfa.c           \
               gen.c

