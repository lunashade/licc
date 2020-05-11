CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

test-all: fmt test test-stage2 test-stage3
lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)

lcc-stage2: lcc $(SRCS) lcc.h self.sh
	./self.sh tmp-stage2 $$PWD/lcc $@
lcc-stage3: lcc-stage2
	./self.sh tmp-stage3 $$PWD/lcc-stage2 $@

test-stage2: lcc-stage2 tests/extern.o
	(cd tests; ../lcc-stage2 tests.c ) > tmp2.s
	cc -static -o tmp2 tmp2.s tests/extern.o
	./tmp2

test-stage3: lcc-stage3
	@diff lcc-stage2 lcc-stage3 && echo "stage3 OK"


$(OBJS): lcc.h

test: fmt lcc tests/extern.o
	(cd tests; ../lcc tests.c ) > tmp.s
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
	rm tmp-* -rf

.PHONY: clean test longtest fmt
