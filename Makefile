# Preliminaries
NAME = blackbox

CC = gcc
CFLAGS = -Wall -Ofast -DNDEBUG
LIBS =
DBCFLAGS = -Wall -O0 -g -DDEBUG

STRIP = strip

# Release
$(NAME): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@

# Debug
debug: main.c
	$(CC) $(DBCFLAGS) -o $(NAME) $< $(LIBS)

# Make tidy
clean:
	$(RM) $(NAME)
