CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Isrc
TEST_BINS := tests/test_telnet tests/test_session tests/test_fragmentation tests/test_terminal tests/test_app_session tests/test_arexx_cmd tests/test_arexx_dispatch tests/test_rx_buffer tests/test_transport_common
CHECK_OBJS := tests/transport_posix.o

.PHONY: all check clean

all: check

tests/test_telnet: tests/test_telnet.c src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_telnet.c src/telnet.c

tests/test_session: tests/test_session.c src/session.c src/session.h src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_session.c src/session.c src/telnet.c

tests/test_fragmentation: tests/test_fragmentation.c src/telnet.c src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_fragmentation.c src/telnet.c

tests/test_terminal: tests/test_terminal.c src/terminal.c src/terminal.h src/telnet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_terminal.c src/terminal.c

tests/test_app_session: tests/test_app_session.c src/app_session.c src/app_session.h src/rx_buffer.c src/rx_buffer.h src/session.c src/session.h src/terminal.c src/terminal.h src/telnet.c src/telnet.h src/transport.h src/transport_common.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_app_session.c src/app_session.c src/rx_buffer.c src/session.c src/terminal.c src/telnet.c src/transport_common.c

tests/test_arexx_cmd: tests/test_arexx_cmd.c src/arexx_cmd.c src/arexx_cmd.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_arexx_cmd.c src/arexx_cmd.c

tests/test_arexx_dispatch: tests/test_arexx_dispatch.c src/arexx_dispatch.c src/arexx_dispatch.h src/arexx_cmd.c src/arexx_cmd.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_arexx_dispatch.c src/arexx_dispatch.c src/arexx_cmd.c

tests/test_rx_buffer: tests/test_rx_buffer.c src/rx_buffer.c src/rx_buffer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_rx_buffer.c src/rx_buffer.c

tests/test_transport_common: tests/test_transport_common.c src/transport_common.c src/transport.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_transport_common.c src/transport_common.c

tests/transport_posix.o: src/transport_posix.c src/transport.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ src/transport_posix.c

check: $(TEST_BINS) $(CHECK_OBJS)
	./tests/test_telnet
	./tests/test_session
	./tests/test_fragmentation
	./tests/test_terminal
	./tests/test_app_session
	./tests/test_arexx_cmd
	./tests/test_arexx_dispatch
	./tests/test_rx_buffer
	./tests/test_transport_common

clean:
	rm -f $(TEST_BINS) $(CHECK_OBJS)
