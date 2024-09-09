CC = gcc
IDIR = include -I libhydrogen  
CFLAGS = -std=c11 -O2 -I $(IDIR)
DEBUGFLAGS = -std=c11 -O0 -g -I $(IDIR) 
LDIR = include
LIBS = -lsqlite3

SDIR = src
ODIR = src/obj
DB_ODIR = src/db_obj
BDIR = bins

VERSION := $(shell cat .version) 

# Define source files and corresponding object files
SRCS = $(wildcard $(SDIR)/*.c) 
OBJS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRCS)) 
DB_OBJS = $(patsubst $(SDIR)/%.c,$(DB_ODIR)/%.o,$(SRCS))

# Default target (all) should build the 'build' rule  
# all: build

# Rule to build the 'build' binary
build: $(OBJS) 
	$(CC) -o $(BDIR)/pm$(VERSION) $^ $(CFLAGS) $(LIBS)

# make debuginfos and don't optimize 
debug: $(DB_OBJS)  
	$(CC) -o $(BDIR)/pm$(VERSION) $^ $(DEBUGFLAGS) $(LIBS) 	

# Rule to compile individual source files into object files
$(ODIR)/%.o: $(SDIR)/%.c $(LDIR)/%.h
	$(CC) -c -o $@ $< $(CFLAGS)

# rule to make object files with debuginfos and no optimization
$(DB_ODIR)/%.o: $(SDIR)/%.c $(LDIR)/%.h
	$(CC) -c -o $@ $< $(DEBUGFLAGS)

# Clean target to remove object files
clean:
	rm -fv $(ODIR)/*.o
	rm -fv $(DB_ODIR)/*.o
#	rm -fv $(ODIR)/*.dSYM 
	
