SRC=main.c
APP=server
TEST=test
CC=gcc -I. -pthread

all:
	$(CC) -g -O0 -o $(APP) $(SRC)
	$(CC) -g -o $(TEST) test.c

test: all 
	./test

clean:
	rm *.o $(APP) $(TEST)

.PHONY: all test

