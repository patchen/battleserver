PORT=11111
CFLAGS = -DPORT=\$(PORT) -g -Wall

battleserver: battleserver.o 
	${CC} ${CFLAGS} -o battleserver  battleserver.o

battleserver.o: battleserver.c battleserver.h
	${CC} ${CFLAGS}  -c $<

clean:
	rm *.o battleserver
