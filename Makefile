CC=gcc
CFLAGS=-g -o2 -Wall -Wextra -Isrc $(OPTFLAGS)
LIBS=-ldl -lpthread $(OPTLIBS)
PREFIX?=/usr/local
LINK=gcc -o

SOURCES=$(wildcard src/**/*.c  src/*.c)
OBJECTS=$(patsubst %.c, objs/%.o, $(SOURCES))


# for test
OBJECTS_WITHOUT_FERVER=$(filter-out objs/src/ferver.o, $(OBJECTS))

TEST_SRC=$(wildcard tests/*_test.c)
TEST=$(patsubst %.c,%,$(TEST_SRC))

TARGET=objs/ferver

all:$(TARGET) tests
ferver:	$(TARGET)

dev: CFLAGS=-g Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: all

$(TARGET): build $(OBJECTS)
	$(LINK) $(TARGET) $(OBJECTS) $(LIBS)

objs/%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

tests/%: tests/%.c
	$(CC) $(CFLAGS) -o $@ $< $(OBJECTS_WITHOUT_FERVER) $(LIBS)

build:
	@mkdir -p objs/src

# 当前目录下有tests这个文件或目录，所以如果不加PHONY，则总是最新的，不会执行下面的指令
.PHONY: tests
tests: $(TESTS)
	@echo "compile test succeed"

valgrind:
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" $(MAKE)

# The cleaner
clean:
	rm -rf objs $(TESTS)
	rm -f tests/tests.log
	rm -rf `find . -name "*dSYM" -print`

# The install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib
