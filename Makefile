.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o

CC = cc
CFLAGS = -std=c17 -Wall -Wextra -O3
LDFLAGS = 
LDLIBS = 
SANFLAGS = -g -01 -fno-omit-frame-pointer -fsanitize=address,undefined \
           -fno-sanitize-recover=undefined


OBJ = paxxfer.o

all: paxxfer

paxxfer: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

san:
	$(MAKE) clean
	$(MAKE) CFLAGS='-std=c17 -Wall -Wextra $(SANFLAGS)' LDFLAGS='$(SANFLAGS)'
