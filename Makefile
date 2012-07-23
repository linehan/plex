CC=gcc
#
#                 
# optimize   warnings
# lvl 3 \     /    
CFLAGS=-O3 -Wall            
LDFLAGS= 
#      
#    
#                                  

SOURCES=main.c scan.c lex.c macro.c nfa.c dfa.c gen.c lib/set.c lib/textutils.c lib/error.c
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=plex

all: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE) 

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) gmon.out 
