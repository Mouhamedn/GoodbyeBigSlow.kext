CC = gcc
CFLAGS = -Wall -Iinclude -D__unused="__attribute__((unused))"

TARGET = test_using_targeted_intel_cpu
SRC = test_using_targeted_intel_cpu.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
