CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

test-all: fmt test test-stage2 test-stage3

$(OBJS): lcc.h

lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)
lcc-stage2: lcc $(SRCS) lcc.h self.sh
	./self.sh tmp-stage2 $$PWD/$< $@
lcc-stage3: lcc-stage2
	./self.sh tmp-stage3 $$PWD/$< $@

test: lcc tests/extern.o
	(cd tests; ../lcc tests.c ) > tmp.s
	cc -static -o tmp tmp.s tests/extern.o
	./tmp
test-stage2: lcc-stage2 tests/extern.o
	(cd tests; ../lcc-stage2 tests.c ) > tmp2.s
	cc -static -o tmp2 tmp2.s tests/extern.o
	./tmp2
test-stage3: lcc-stage3
	@diff lcc-stage2 lcc-stage3 && echo "stage3 OK"

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all
