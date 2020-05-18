CFLAGS=-std=c11 -g -fno-common
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
	$< tests/tests.c -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-nopic: bin/lcc tests/extern.o
	$< tests/tests.c -c -o tmp.o -fno-pic
	cc -static -o tmp tmp.o tests/extern.o
	./tmp
test-stage2: bin/lcc-stage2 tests/extern.o
	$< tests/tests.c -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-stage3: bin/lcc-stage3
	@diff bin/lcc-stage2 bin/lcc-stage3 && echo "stage3 OK"

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all
