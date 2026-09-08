CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Isrc
TEST_BIN := tests/test_telnet

.PHONY: all check clean

all: check

$(TEST_BIN): tests/test_telnet.c src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_telnet.c src/telnet.c

check: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN)
