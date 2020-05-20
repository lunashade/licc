CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)
INCLUDES=$(wildcard include/*.h)

test-all: fmt test test-nopic test-stage2 test-stage3

$(OBJS): src/licc.h

bin/include: $(INCLUDES)
	@mkdir -p $@
	@cp $^ --target-directory=$@
bin/licc: $(OBJS) bin/include
	@mkdir bin -p
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
bin/licc-stage2: bin/licc $(SRCS) src/licc.h self.sh bin/include
	@mkdir bin -p
	./self.sh $(patsubst bin/licc-%,tmp-%,$@) $$PWD/$< $@
bin/licc-stage3: bin/licc-stage2 bin/include
	@mkdir bin -p
	./self.sh $(patsubst bin/licc-%,tmp-%,$@) $$PWD/$< $@

test: bin/licc tests/extern.o
	$< tests/tests.c -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-nopic: bin/licc tests/extern.o
	$< tests/tests.c -c -o tmp.o -fno-pic
	cc -static -o tmp tmp.o tests/extern.o
	./tmp
test-stage2: bin/licc-stage2 tests/extern.o
	$< tests/tests.c -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-stage3: bin/licc-stage3
	@diff bin/licc-stage2 bin/licc-stage3 && echo "stage3 OK"

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm bin/* tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all
