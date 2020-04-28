CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: test
lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)

$(OBJS): lcc.h

test: lcc
	# tests/old_test.sh
	./lcc tests/tests.c > tmp.s
	cc -static -o tmp tmp.s
	./tmp

longtest: clean lcc test
	tests/old_test.sh

clean:
	git clean -fX

.PHONY: clean test longtest
