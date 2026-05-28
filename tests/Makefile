CXX = g++
CXXFLAGS = -Wall -Iinclude -D__unused="__attribute__((unused))" -std=c++11

TARGET = test_using_targeted_intel_cpu
SRC = test_using_targeted_intel_cpu.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
