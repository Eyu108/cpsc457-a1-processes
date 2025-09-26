# Simple Makefile for A1 (parts I & II)
# Use: make            -> build a1p1 and a1p2
#      make DEBUG=1    -> build with -DDEBUG (extra prints)
#      make p1         -> run a1p1 on test1.txt..test6.txt if present
#      make p2         -> example run for part II
#      make clean

CC      := gcc
CFLAGS  ?= -O2 -Wall
LDLIBS  := -lm
SRC_DIR := src

ifeq ($(DEBUG),1)
  CFLAGS += -DDEBUG
endif

.PHONY: all clean p1 p2

all: a1p1 a1p2

a1p1: $(SRC_DIR)/a1p1.c
	$(CC) $(CFLAGS) $< -o $@

a1p2: $(SRC_DIR)/a1p2.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# Run Part I on all provided tests it can find (root, inputs/, tests/, data/)
p1: a1p1
	@set -e; \
	files=""; \
	for d in . inputs tests data; do \
	  for n in 1 2 3 4 5 6; do \
	    [ -f $$d/test$$n.txt ] && files="$$files $$d/test$$n.txt"; \
	  done; \
	done; \
	if [ -z "$$files" ]; then echo "No testN.txt files found."; exit 1; fi; \
	for f in $$files; do \
	  echo "== ./a1p1 < $$f =="; \
	  ./a1p1 < "$$f"; \
	done

# Example Part II run (adjust as needed)
p2: a1p2
	@echo "== ./a1p2 100 249 3 =="; ./a1p2 100 249 3

clean:
	rm -f a1p1 a1p2 *.o
