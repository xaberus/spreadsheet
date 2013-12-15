
# Automatic variables:
# $@ is the file name of the target of the rule
# $< is the name of the first dependency of the rule
# $^ is the list of all dependencies of the rule
# $? is the list of dependencies that are newer than the target
#
# Substitution references:
# OBJS = $(SRCS:.c=.o)
# example: if $SRCS is "foo.c bar.c" then $OBJS will be "foo.o bar.o"
# http://www.gnu.org/software/make/manual/make.html#Substitution-Refs
#
# GNU make documentation
# http://www.gnu.org/software/make/manual/make.html
#
# GCC options
# http://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html#Option-Summary


CC=gcc
LINKER=gcc

CPPFLAGS= -I./contrib
CFLAGS=-Wall -Wextra -std=gnu99 $(CPPFLAGS)
DEBUG=-ggdb
LDFLAGS=-lm

EXES=test spreadsheet

TEST_SRCS=src/test.c
TEST_OBJS=$(patsubst %.c,objs/%.o,$(TEST_SRCS))

SPREADSHEET_SRCS=src/main.c contrib/linenoise/linenoise.c
SPREADSHEET_OBJS=$(patsubst %.c, objs/%.o,$(SPREADSHEET_SRCS))

COMMON_SRCS=\
	src/Call.c \
	src/Cell.c \
	src/Coord.c \
	src/Double.c \
	src/List.c \
	src/Object.c \
	src/Range.c \
	src/String.c \
	src/Table.c \
	src/TableDispatch.c \
	src/Buffer.c \
	src/Formula.c \
	src/Pattern.c \
	src/Error.c \
	src/DebugPrintf.c \
	src/Strtab.c \
	src/Collector.c
COMMON_OBJS=$(patsubst %.c,objs/%.o,$(COMMON_SRCS))

SRCS=\
	$(TEST_SRCS) \
	$(SPREADSHEET_SRCS) \
	$(COMMON_SRCS)
OBJS=$(patsubst %.c,objs/%.o,$(SRCS))
DEPS=$(patsubst %.c,deps/%.d,$(SRCS))


.SUFFIXES:

all: $(EXES)

deps/%.d: %.c
	@mkdir -p `dirname $@`
	@set -e; rm -f deps/$@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$ ;\
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ ;\
		rm -f $@.$$$$;

-include $(DEPS)

objs/%.o: %.c Makefile
	@mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) $(DEBUG) $< -o $@

.PHONY: clean
clean:
	rm -f $(EXES)
	rm -f $(OBJS)
	rm -f $(DEPS)

.PHONY: zip
zip: $(NAME).zip

spreadsheet: $(SPREADSHEET_OBJS) $(COMMON_OBJS)
	$(LINKER) -o $@ $(LDFLAGS) $^

test: $(TEST_OBJS) $(COMMON_OBJS)
	$(LINKER) -o $@ $(LDFLAGS) $^
