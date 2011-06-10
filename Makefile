SUBDIRS = src
CADEIRA = RIC
PROJECTO = ATM
ALUNO1 = 57442
ALUNO2 = 57476
CC = gcc
CFLAGS = -g -O0 -Wall


all: build install

build:
	@list='$(SUBDIRS)'; for p in $$list; do \
	  echo "Building $$p"; \
	  $(MAKE) -C $$p; \
	done

install:
	mv src/strs bin/
	mv src/strc bin/	
	mv src/strs0 bin/
	mv src/strc0 bin/
	ln -s bin/strs strs 	
	ln -s bin/strc strc
	ln -s bin/strs0 strs0		
	ln -s bin/strc0 strc0

clean:
	@list='$(SUBDIRS)'; for p in $$list; do \
	  echo "Cleaning $$p"; \
	  $(MAKE) clean -C $$p; \
	done
	rm -f bin/strs
	rm -f bin/strc
	rm -f strs
	rm -f strc
	rm -f bin/strs0
	rm -f bin/strc0	
	rm -f strs0
	rm -f strc0

package: clean zip

zip:
	zip -r ../$(CADEIRA)-$(PROJECTO)-$(ALUNO1)-$(ALUNO2).zip *
