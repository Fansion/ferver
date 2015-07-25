CC=gcc
CFLAGS=-g -o2 -Wall -Wextra -Isrc $(OPTFLAGS)
LIBS=-ldl -lpthread $(OPTLIBS)
LINK=gcc -o

SOURCES=$(wildcard src/**/*.c  src/*.c)
OBJECTS=$(patsubst %.c, objs/%.o, $(SOURCES))


# for test
OBJECTS_WITHOUT_FERVER=$(filter-out objs/src/ferver.o, $(OBJECTS))

TEST_SRC=$(wildcard tests/*_test.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=objs/ferver

all:$(TARGET) tests
ferver:	$(TARGET)

dev: CFLAGS=-g Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: all

$(TARGET): build $(OBJECTS)
	$(LINK) $(TARGET) $(OBJECTS) $(LIBS)
	@echo "compile source succeed"

objs/%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

tests/%: tests/%.c
	$(CC) $(CFLAGS) -o $@ $< $(OBJECTS_WITHOUT_FERVER) $(LIBS)

build:
	@mkdir -p objs/src

.PHONY: tests
tests: $(TESTS)
	@echo "compile test succeed"

# The cleaner
clean:
	rm -rf objs $(TESTS)
	rm -rf `find . -name "*dSYM" -print`
