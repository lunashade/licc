CFLAGS=-std=c11 -g -static -fno-common
LCCFLAGS=-I.
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

test-all: fmt test test-stage2 test-stage3

$(OBJS): src/lcc.h

lcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
lcc-stage2: lcc $(SRCS) src/lcc.h self.sh
	./self.sh $(patsubst lcc-%,tmp-%,$@) $$PWD/$< $@
lcc-stage3: lcc-stage2
	./self.sh $(patsubst lcc-%,tmp-%,$@) $$PWD/$< $@

test: lcc tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp.s
	cc -static -o tmp tmp.s tests/extern.o
	./tmp
test-stage2: lcc-stage2 tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp2.s
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
