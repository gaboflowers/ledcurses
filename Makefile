CC:= gcc
SRC:= ./src/ledcurses.c
LIBS:= -lncurses
LIBDIR:= ./lib
EXAMPLES:= $(wildcard examples/*.c)
TARGETS:=  $(patsubst %.c,%,$(EXAMPLES))
STAT_TARGETS:= $(foreach bin,$(TARGETS),$(bin).static)
INCLUDES:= -I./include

.PHONY: all lib
all:	$(TARGETS)

$(TARGETS): lib
	$(CC) -Wall $(INCLUDES) -o $@.static $(LIBS) $(LIBDIR)/ledcurses.o $@.c
	$(CC) -Wall $(INCLUDES) -o $@ $(LIBS) -L$(LIBDIR) -ledcurses $@.c


lib: $(LIBDIR)/ledcurses.o
	$(CC) -Wall $(INCLUDES) -shared -o ./lib/libedcurses.so $^

$(LIBDIR)/ledcurses.o:
	$(CC) $(LIBS) $(INCLUDES) -c -fPIC $(SRC) -o $(LIBDIR)/ledcurses.o

clean:
	rm -f ./lib/libedcurses.so ./lib/ledcurses.o
	rm -f $(TARGETS) $(STAT_TARGETS)
