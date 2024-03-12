CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c11

main: main.o cpu.o
	$(CC) $(CFLAGS) -o main main.o cpu.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

clean:
	rm *.o
