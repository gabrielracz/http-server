all: srv cli
CFLAGS= -Og
LIBS= -lpthread

srv: src/server.c src/defines.h
		gcc ${CFLAGS} -o srv src/server.c ${LIBS}

cli: src/client.c src/defines.h
		gcc ${CFLAGS} -o cli src/client.c ${LIBS}

clean:
	rm build/* cli srv
