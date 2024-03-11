CC=clang
CFLAGS=-Wall -Wextra -Werror -g -ggdb

main: main.o cpu.o rom.o
	$(CC) $(CFLAGS) -o main main.o cpu.o rom.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

rom.o: rom.c
	$(CC) $(CFLAGS) -c rom.c -o rom.o

clean:
	rm *.o