CC = gcc
CFLAGS = -pthread -Ilib
LDFLAGS = -pthread

.PHONY: clean

router: router.o handlers.o threads.o hm.o tools.o lsa.o

debug: CFLAGS += -DDEBUG
debug: router

ui: ui.o tools.o

naming: naming.c
	$(CC) $^ -lcurl -o $@

clean:
	$(RM) *.o router ui
