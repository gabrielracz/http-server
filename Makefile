OBJECTS= build/http.o build/server.o build/perlin.o build/picohttpparser.o build/logger.o build/content.o
all: srv ${OBJECTS}
CFLAGS= -O2 -static
LIBS= -lpthread -lm


build/content.o: src/content.c src/content.h
	gcc ${CFLAGS} -c src/content.c -o build/content.o

build/http.o: src/http.c src/http.h src/content.c src/content.h
	gcc ${CFLAGS} -c src/http.c -o build/http.o

build/perlin.o: src/perlin.c src/perlin.h
	gcc ${CFLAGS} -c src/perlin.c -o build/perlin.o

build/picohttpparser.o: http-parser/picohttpparser.c http-parser/picohttpparser.h
	gcc ${CFLAGS} -c http-parser/picohttpparser.c -o build/picohttpparser.o

build/logger.o: src/logger.c src/logger.h
	gcc ${CFLAGS} -c src/logger.c -o build/logger.o

build/server.o: src/server.c src/server.h build/http.o build/perlin.o build/logger.o build/content.o
	gcc ${CFLAGS} -c src/server.c -o build/server.o

srv: ${OBJECTS}
	gcc ${CFLAGS} -o srv src/main.c ${OBJECTS} ${LIBS}

clean:
	rm cli srv build/*
