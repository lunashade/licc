CFLAGS=-std=c11 -g -static -fno-common
LCCFLAGS=-I.
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

test-all: fmt test test-stage2 test-stage3 test-pp test-stage2-pp

$(OBJS): lcc.h

lcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
lcc-stage2: lcc $(SRCS) lcc.h self.sh
	./self.sh $(patsubst lcc-%,tmp-%,$@) $$PWD/$< $@
lcc-stage3: lcc-stage2
	./self.sh $(patsubst lcc-%,tmp-%,$@) $$PWD/$< $@

test: lcc tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp.s
	cc -static -o tmp tmp.s tests/extern.o
	./tmp
test-pp: lcc
	(cd tests; ../$< $(LCCFLAGS) pptests.c ) > tmp-pp.s
	cc -static -o tmp-pp tmp-pp.s
	./tmp-pp
test-stage2: lcc-stage2 tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp2.s
	cc -static -o tmp2 tmp2.s tests/extern.o
	./tmp2
test-stage2-pp: lcc-stage2
	(cd tests; ../$< $(LCCFLAGS) pptests.c ) > tmp2-pp.s
	cc -static -o tmp2-pp tmp2-pp.s
	./tmp-pp
test-stage3: lcc-stage3
	@diff lcc-stage2 lcc-stage3 && echo "stage3 OK"

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all
