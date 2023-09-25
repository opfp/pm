CC = gcc
IDIR = include -I libhydrogen  
CFLAGS = -std=c11 -I $(IDIR)
LDIR = include
LIBS = -lsqlite3

SDIR = src
ODIR = src/obj

# Define source files and corresponding object files
SRCS = $(wildcard $(SDIR)/*.c) 
# libhydrogen/libhydrogen.c  
OBJS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRCS)) 
#$(ODIR)/libhydrogen.o 

# Default target (all) should build the 'build' binary
all: build

# Rule to build the 'build' binary
build: $(OBJS) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Rule to compile individual source files into object files
$(ODIR)/%.o: $(SDIR)/%.c $(LDIR)/%.h
	$(CC) -c -o $@ $< $(CFLAGS)

#$(ODIR)/libhydrogen.o: 

# Clean target to remove object files
clean:
	rm -fv $(ODIR)/*.o
