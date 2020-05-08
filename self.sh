#!/bin/bash
set -e

TMP=tmp-stage2

mkdir -p $TMP

lcc() {
    cat <<EOF > $TMP/$1
typedef struct FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

void *malloc(long size);
void *calloc(long nmemb, long size);
void *realloc(void *buf, long size);
int *__errno_location();
char *strerror(int errnum);
FILE *fopen(char *pathname, char *mode);
long fread(void *ptr, long size, long nmemb, FILE *stream);
int fclose(FILE *fp);
int feof(FILE *stream);
static void assert() {}
int strcmp(char *s1, char *s2);
int printf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);
long strlen(char *p);
int strncmp(char *p, char *q);
void *memcpy(char *dst, char *src, long n);
char *strndup(char *p, long n);
int isspace(int c);
int iscntrl(int c);
int ispunct(int c);
char *strstr(char *haystack, char *needle);
EOF

    grep -v '^#' lcc.h >> $TMP/$1
    grep -v '^#' $1 >> $TMP/$1
    sed -i 's/\bbool\b/_Bool/g' $TMP/$1
    sed -i 's/\berrno\b/*__errno_location()/g' $TMP/$1
    sed -i 's/\btrue\b/1/g; s/\bfalse\b/0/g;' $TMP/$1
    sed -i 's/\bNULL\b/0/g' $TMP/$1
    sed -i 's/, \.\.\.//g' $TMP/$1

    ./lcc $TMP/$1 > $TMP/${1%.c}.s
    gcc -c -o $TMP/${1%.c}.o $TMP/${1%.c}.s
}

cc() {
    gcc -c -o $TMP/${1%.c}.o $1
}

lcc main.c
lcc typing.c
cc parse.c
lcc codegen.c
cc tokenize.c

gcc -static -o lcc-stage2 $TMP/*.o
