OBJECTS= build/server.o build/perlin.o build/picohttpparser.o 
all: srv cli ${OBJECTS}
CFLAGS= -g
LIBS= -lpthread -lm

build/perlin.o: src/perlin.c src/perlin.h
	gcc -c src/perlin.c -o build/perlin.o

build/picohttpparser.o: http-parser/picohttpparser.c http-parser/picohttpparser.h
	gcc ${CFLAGS} -c http-parser/picohttpparser.c -o build/picohttpparser.o

build/server.o: src/server.c src/http.h src/perlin.o
	gcc -c src/server.c -o build/server.o

srv: ${OBJECTS}
	gcc ${CFLAGS} -o srv src/main.c ${OBJECTS} ${LIBS}

cli: src/client.c src/universal.h
	gcc ${CFLAGS} -o cli src/client.c ${LIBS}

clean:
	rm cli srv build/*
