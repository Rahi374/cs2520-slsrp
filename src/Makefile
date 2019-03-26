CC = gcc
CFLAGS = -pthread -Ilib -g
LDFLAGS = -pthread
LDLIBS = -lcurl

.PHONY: clean

router: router.o handlers.o db.o threads.o hm.o tools.o lsa.o naming.o

debug: CFLAGS += -DDEBUG -g
debug: router

ui: ui.o tools.o naming.o

clean:
	$(RM) *.o router ui
