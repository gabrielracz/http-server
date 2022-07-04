all: srv cli phttp/picohttpparser.o
CFLAGS= -Og
LIBS= -lpthread

srv: src/server.c src/defines.h
		gcc ${CFLAGS} -o srv src/server.c phttp/picohttpparser.o ${LIBS}

cli: src/client.c src/defines.h
		gcc ${CFLAGS} -o cli src/client.c ${LIBS}

phttp/picohttpparser.o: phttp/picohttpparser.c phttp/picohttpparser.h
	gcc ${CFLAGS} -c phttp/picohttpparser.c -o phttp/picohttpparser.o

clean:
	rm build/* cli srv
