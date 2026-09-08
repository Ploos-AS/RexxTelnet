CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Isrc
TEST_BINS := tests/test_telnet tests/test_session

.PHONY: all check clean

all: check

tests/test_telnet: tests/test_telnet.c src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_telnet.c src/telnet.c

tests/test_session: tests/test_session.c src/session.c src/session.h src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_session.c src/session.c src/telnet.c

check: $(TEST_BINS)
	./tests/test_telnet
	./tests/test_session

clean:
	rm -f $(TEST_BINS)
