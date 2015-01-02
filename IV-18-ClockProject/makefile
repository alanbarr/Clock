CC=msp430-gcc
CCFLAGS=-Wall -g -Os
TARGET=msp430g2553
OUTPUT=vfd_clock.elf
all: clean $(OUTPUT)

SRC= main.c

$(OUTPUT):
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -o $(OUTPUT) $(SRC)

preprocess:
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -E $(SRC)

compile:
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -S $(SRC)

clean:
	-rm $(OUTPUT)
	
