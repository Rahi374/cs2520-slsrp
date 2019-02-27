CC = gcc
CFLAGS = -pthread -Ilib
LDFLAGS = -pthread

router: router.o handlers.o threads.o hm.o tools.o

ui: ui.c
	$(CC) -o $@ $(CFLAGS) $^
