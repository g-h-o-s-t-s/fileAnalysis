CC = gcc
CFLAGS = -g -Wall

all: greet generate

greet:
	@echo "Now running MAKEFILE.."

generate: Asst2.c
	@echo "Generating exe for Asst2.c..."
	$(CC) $(CFLAGS) Asst2.c -o detector
  
clean:
	@echo "Cleaning up..."
	rm -f detector
