CC = gcc
CFLAGS = -pthread -Ilib
LDFLAGS = -pthread

.PHONY: clean

router: router.o handlers.o threads.o hm.o tools.o

ui: ui.o tools.o

clean:
	$(RM) *.o router ui
