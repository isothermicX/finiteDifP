CC = gcc
CFLAGS = -Wall -fopenmp
LFLAGS = -lm -lpapi -fopenmp

all: finiteDif_serial finiteDifP

finiteDif_serial:	finiteDif_serial.o
	$(CC) -o finiteDif_serial finiteDif_serial.o $(LFLAGS)

finiteDif_serial.o: finiteDif_serial.c
	$(CC) $(CFLAGS) -c finiteDif_serial.c

finiteDifP:	finiteDifP.o
	$(CC) -o finiteDifP finiteDifP.o $(LFLAGS)

finiteDifP.o:	finiteDifP.c
	$(CC) $(CFLAGS) -c finiteDifP.c

clean:
	rm -f *~ *.o finiteDif_serial finiteDifP

submit:
	tar -czvf finiteDifP.tar.gz Makefile README *.c

.PHONY: clean submit