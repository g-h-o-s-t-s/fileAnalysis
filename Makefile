CC = gcc
PREFLAGS = -Wall -o
POSTFLAGS = -pthread -lm

all: greet run

greet:
	@echo "Now running MAKEFILE..."

run: Asst2.c
	@echo "Generating detector.exe from Asst2.c..."
	$(CC) $(PREFLAGS) detector Asst2.c $(POSTFLAGS)

clean:
	@echo "Cleaning up..."
	rm -f detector