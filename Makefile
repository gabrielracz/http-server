OBJECTS= build/server.o build/perlin.o build/picohttpparser.o build/logger.o
all: srv cli ${OBJECTS}
CFLAGS= -O2 -static
LIBS= -lpthread -lm

build/perlin.o: src/perlin.c src/perlin.h
	gcc ${CFLAGS} -c src/perlin.c -o build/perlin.o

build/picohttpparser.o: http-parser/picohttpparser.c http-parser/picohttpparser.h
	gcc ${CFLAGS} -c http-parser/picohttpparser.c -o build/picohttpparser.o

build/logger.o: src/logger.c src/logger.h
	gcc ${CFLAGS} -c src/logger.c -o build/logger.o

build/server.o: src/server.c src/http.h src/perlin.o
	gcc ${CFLAGS} -c src/server.c -o build/server.o

srv: ${OBJECTS}
	gcc ${CFLAGS} -o srv src/main.c ${OBJECTS} ${LIBS}

cli: src/client.c src/universal.h
	gcc ${CFLAGS} -o cli src/client.c ${LIBS}

clean:
	rm cli srv build/*
