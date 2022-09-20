all: perlin.o phttp/picohttpparser.o srv cli 
CFLAGS= -g
LIBS= -lpthread -lm

perlin.o: src/perlin.c src/perlin.h
	gcc -c src/perlin.c -o build/perlin.o

srv: src/server.c src/universal.h src/http.h src/perlin.o
	gcc ${CFLAGS} -o srv src/server.c build/perlin.o phttp/picohttpparser.o ${LIBS}

cli: src/client.c src/universal.h
	gcc ${CFLAGS} -o cli src/client.c ${LIBS}

phttp/picohttpparser.o: phttp/picohttpparser.c phttp/picohttpparser.h
	gcc ${CFLAGS} -c phttp/picohttpparser.c -o phttp/picohttpparser.o

clean:
	rm cli srv
