CFLAGS=-std=c11 -g -static -fPIC
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: clean test
lcc: $(OBJS)
	$(CC) -o lcc $(OBJS) $(LDFLAGS)

$(OBJS): lcc.h

test: lcc
	./test.sh

clean:
	git clean -fX

.PHONY: clean test
