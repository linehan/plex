CC=gcc
#
#                 
# optimize  warning gnu debugger
# lvl 3 \   |      /
CFLAGS=-O3 -Wall -g 
LDFLAGS= 
#      
#    
#                                  

SOURCES=main.c lib/file.c lib/set.c lib/textutils.c lib/debug.c scan.c lex.c macro.c nfa.c dfa.c gen.c
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=plex

all: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE) 

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) gmon.out 
