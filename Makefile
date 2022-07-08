all: phttp/picohttpparser.o build/http.o srv cli 
CFLAGS= -g
LIBS= -lpthread

srv: src/server.c src/universal.h
	gcc ${CFLAGS} -o srv src/server.c phttp/picohttpparser.o build/http.o ${LIBS}

cli: src/client.c src/universal.h
	gcc ${CFLAGS} -o cli src/client.c ${LIBS}

build/http.o: src/http.c src/http.h
	gcc ${CFLAGS} -o build/http.o -c src/http.c ${LIBS}

phttp/picohttpparser.o: phttp/picohttpparser.c phttp/picohttpparser.h
	gcc ${CFLAGS} -c phttp/picohttpparser.c -o phttp/picohttpparser.o

clean:
	rm build/* cli srv
