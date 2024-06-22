#
# Makefile
#
# Targets:
# 	all
#       The default target, if no target is specified. Compiles source files
#       as necessary and links them into the final executable.
#   bear
#	   Generates a compile_commands.json file for use with LSPs.
#   clean
#       Removes all object files and executables.
#   install
#	   Installs md2jl into the directory specified by the prefix variable. 
#	   Defaults to /usr/local.
#	uninstall
#	   Removes md2jl from bin.
# Variables:
#   CC
#	   The C compiler to use. Defaults to gcc.
#   CFLAGS
#	   Flags to pass to the C compiler. Defaults to -I/usr/include. If libjson-c
#	   is installed in a non-standard location, you may need to
#	   add -I/path/to/include to this variable. On Mac OS, for example, I use
#	   MacPorts, so I use make CFLAGS="-I/opt/local/include".
#   debug=1
#       Build md2jl with debug info
#   LDFLAGS
#	   Flags to pass to the linker. Defaults to -L/usr/lib. If libjson-c is 
#	   installed in a non-standard location, you may need to add -L/path/to/lib
#	   to this variable. On Mac OS, for example, I use MacPorts, so I use make 
#	   LDFLAGS="-L/opt/local/lib".
#   prefix
#	   The directory to install md2jl into. Defaults to /usr/local. md2jl is
#	   installed into $(prefix)/bin.
#

ifeq ($(CC),)
CC = gcc
endif
CFLAGS += -I/usr/include
LDFLAGS += -L/usr/lib
ifdef debug
CFLAGS += -g3 -D DEBUG
else
CFLAGS += -O3 -flto
LDFLAGS += -flto
endif
ifeq ($(prefix),)
prefix = /usr/local
endif

LIBS = -ljson-c

OBJS = main.o

.PHONY: all bear clean install uninstall

all: ld2json

bear:
	make clean
	bear -- make

clean:
	- rm -f ld2json
	- rm -f *.o

ld2json : $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@
ifndef debug
	strip $@
endif

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

install : ld2json
	install -m 755 ld2json $(prefix)/bin

uninstall :
	- rm -f $(prefix)/bin/ld2json

