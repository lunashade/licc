CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: test
lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)

$(OBJS): lcc.h

test: lcc fmt
	# tests/old_test.sh
	./lcc tests/tests.c > tmp.s
	echo 'int static_fn() {return 5;}' | gcc -xc -c -o tmp2.o -
	cc -static -o tmp tmp.s tmp2.o
	./tmp

test-nqueen: lcc
	./lcc examples/nqueen.c > tmp-nqueen.s
	cc -static -o tmp-nqueen tmp-nqueen.s
	./tmp-nqueen

test-dp: lcc
	@examples/dp_a.sh

fmt:
	@bash fmt.sh
clean:
	git clean -fX

.PHONY: clean test longtest fmt
