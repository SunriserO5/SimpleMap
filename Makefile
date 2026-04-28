CXX := /usr/bin/clang++
CXXFLAGS := -std=gnu++14 -Wall -Wextra -pedantic -g

MAIN_TARGET := main
MAIN_SOURCES := main.cpp dataStructure.cpp pathfinding.cpp

TEST_DATA_TARGET := test_data_generator
TEST_DATA_SOURCES := test_data_generator.cpp dataStructure.cpp pathfinding.cpp

.PHONY: all clean test-data

all: $(MAIN_TARGET)

$(MAIN_TARGET): $(MAIN_SOURCES)
	$(CXX) $(CXXFLAGS) $(MAIN_SOURCES) -o $(MAIN_TARGET)

test-data: $(TEST_DATA_TARGET)

$(TEST_DATA_TARGET): $(TEST_DATA_SOURCES)
	$(CXX) $(CXXFLAGS) $(TEST_DATA_SOURCES) -o $(TEST_DATA_TARGET)

clean:
	rm -f $(MAIN_TARGET) $(TEST_DATA_TARGET)
