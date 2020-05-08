CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: fmt test
lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)

$(OBJS): lcc.h

test: lcc tests/extern.o
	./lcc tests/tests.c > tmp.s
	cc -static -o tmp tmp.s tests/extern.o
	./tmp

test-nqueen: lcc
	./lcc examples/nqueen.c > tmp-nqueen.s
	cc -static -o tmp-nqueen tmp-nqueen.s
	./tmp-nqueen

test-dp: lcc
	@examples/dp_a.sh

fmt:
	@tests/fmt.sh
clean:
	git clean -fX

.PHONY: clean test longtest fmt
