CFLAGS=-std=c11 -g -static

lcc: lcc.c

test: lcc
	./test.sh

clean:
	git clean -fX

.PHONY: clean test
