CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

TARGET = cache_test
SRC = src/Cache.cc src/Main.cc

# Default cache configuration
NUM_SETS ?= 16
NUM_WAYS ?= 4
BLOCK_SIZE ?= 64

all: $(TARGET)

$(TARGET): $(SRC) include/Cache.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Pass cache configuration as command-line arguments
run: $(TARGET)
	@echo "Running cache simulator with NUM_SETS=$(NUM_SETS), NUM_WAYS=$(NUM_WAYS), BLOCK_SIZE=$(BLOCK_SIZE)"
	./$(TARGET) $(NUM_SETS) $(NUM_WAYS) $(BLOCK_SIZE)

clean:
	rm -f $(TARGET)

