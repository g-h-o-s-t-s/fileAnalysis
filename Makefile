CC = gcc
PREFLAGS = -Wall -o
POSTFLAGS = -pthread -lm

all: greet run

greet:
	@echo "Now running MAKEFILE..."

run: Asst2.c
	@echo "Generating detector.exe from FileAnalysis.c..."
	$(CC) $(PREFLAGS) detector FileAnalysis.c $(POSTFLAGS)

clean:
	@echo "Cleaning up..."
	rm -f detector
