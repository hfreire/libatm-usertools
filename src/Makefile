SOURCES= strc strs strc0 strs0

INCLUDES = -I. -I$(srcdir) -I../include -I ../include
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS)
CC = gcc
CFLAGS = -I. -g -Wall -Wshadow -Wpointer-arith -Wwrite-strings -Wno-strict-prototypes
#LDLIBS = /usr/local/lib/libatm.so -lresolv -lm -Wl,--rpath -Wl,/usr/local/lib
LDLIBS = /usr/lib/libatm.so -lresolv -lm -Wl,--rpath -Wl,/usr/lib

all: $(SOURCES)

.SUFFIXES: .c .o

strc: strc.o
	$(CC) $(CFLAGS) -lm $(LDFLAGS) -o $@ strc.o $(LDLIBS)

strc0: strc0.o crc32h.o
	$(CC) $(CFLAGS) -lm $(LDFLAGS) -o $@ strc0.o crc32h.o $(LDLIBS)

strs: strs.o
	$(CC) $(CFLAGS) -lm $(LDFLAGS) -o $@ strs.o $(LDLIBS)

strs0: strs0.o crc32h.o
	$(CC) $(CFLAGS) -lm $(LDFLAGS) -o $@ strs0.o crc32h.o $(LDLIBS)

.c.o:   ../strs.h
	$(COMPILE) -c -o $@ $<

clean: clean-SOURCES
	rm -f *.o

clean-SOURCES:
	@list='$(SOURCES)'; for p in $$list; do \
	  f=`echo $$p|sed 's/$$//'`; \
	  echo " rm -f $$p $$f"; \
	  rm -f $$p $$f ; \
	done


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
