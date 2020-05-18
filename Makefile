CFLAGS=-std=c11 -g -fno-common
LCCFLAGS=-I. -o -
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

test-all: fmt test test-nopic test-stage2 test-stage3

$(OBJS): src/lcc.h

bin/lcc: $(OBJS)
	@mkdir bin -p
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
bin/lcc-stage2: bin/lcc $(SRCS) src/lcc.h self.sh
	@mkdir bin -p
	./self.sh $(patsubst bin/lcc-%,tmp-%,$@) $$PWD/$< $@
bin/lcc-stage3: bin/lcc-stage2
	@mkdir bin -p
	./self.sh $(patsubst bin/lcc-%,tmp-%,$@) $$PWD/$< $@

test: bin/lcc tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp.s
	cc -o tmp tmp.s tests/extern.o
	./tmp
test-nopic: bin/lcc tests/extern.o
	(cd tests; ../$< -fno-pic $(LCCFLAGS) tests.c ) > tmp.s
	cc -static -o tmp tmp.s tests/extern.o
	./tmp
test-stage2: bin/lcc-stage2 tests/extern.o
	(cd tests; ../$< $(LCCFLAGS) tests.c ) > tmp2.s
	cc -o tmp2 tmp2.s tests/extern.o
	./tmp2
test-stage3: bin/lcc-stage3
	@diff bin/lcc-stage2 bin/lcc-stage3 && echo "stage3 OK"

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all
