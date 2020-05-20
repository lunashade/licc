CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)
INCLUDES=$(wildcard include/*.h)

SRCDIR=$(CURDIR)$(.CURDIR)

PREFIX=${HOME}/.local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib

LIBDIR_SOURCE=$(SRCDIR)/bin
LIBDIR_TARGET=$(LIBDIR)/licc

test-all: fmt test test-nopic test-stage2 test-stage3

$(OBJS): src/licc.h

bin/include: $(INCLUDES)
	@mkdir -p $@
	@cp $^ --target-directory=$@
bin/licc: $(OBJS) bin/include
	@mkdir -p $(@D)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
bin/licc-stage2: bin/licc $(SRCS) src/licc.h bin/include
	@mkdir -p $(@D)
	@mkdir -p $(patsubst bin/licc-%,tmp-%,$@)
	for src in $(SRCS); do \
		$< -c $$src -o \
		"$(patsubst bin/licc-%,tmp-%,$@)/$$(basename $$src .c).o"; \
	done
	(cd $(patsubst bin/licc-%,tmp-%,$@); gcc -o ../$@ *.o)
bin/licc-stage3: bin/licc-stage2 bin/include
	@mkdir -p $(@D)
	@mkdir -p $(patsubst bin/licc-%,tmp-%,$@)
	for src in $(SRCS); do \
		$< -c $$src -o \
		"$(patsubst bin/licc-%,tmp-%,$@)/$$(basename $$src .c).o"; \
	done
	(cd $(patsubst bin/licc-%,tmp-%,$@); gcc -o ../$@ *.o)

bin/release/licc: bin/licc bin/include
	@mkdir -p $(@D)
	$(CC) -o $@ $(CFLAGS) -D'LICC_LIB_DIR="$(LIBDIR_TARGET)"' $(SRCS) $(LDFLAGS)

test: bin/licc tests/extern.o
	$< tests/tests.c -DANSWER=42 -DDMACRO -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-nopic: bin/licc tests/extern.o
	$< tests/tests.c -DANSWER=42 -DDMACRO -c -o tmp.o -fno-pic
	cc -static -o tmp tmp.o tests/extern.o
	./tmp
test-stage2: bin/licc-stage2 tests/extern.o
	$< tests/tests.c -DANSWER=42 -DDMACRO -c -o tmp.o
	cc -o tmp tmp.o tests/extern.o
	./tmp
test-stage3: bin/licc-stage3
	@diff bin/licc-stage2 bin/licc-stage3 && echo "stage3 OK"

install: bin/release/licc
	mkdir -p $(LIBDIR_TARGET)/include
	cp -r $(LIBDIR_SOURCE)/include/*.h $(LIBDIR_TARGET)/include/
	cp $? $(BINDIR)/licc

uninstall:
	rm -rf $(LIBDIR_TARGET)
	rm $(BINDIR)/licc

fmt:
	clang-format -i $(SRCS)
	@tests/fmt.sh
clean:
	git clean -fX
	rm bin/* tmp-* -rf

.PHONY: clean fmt test test-stage2 test-stage3 test-all \
		install uninstall
