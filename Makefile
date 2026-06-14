CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Isrc
TARGET   = jsrun
SRC      = src/main.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC) src/lexer.h src/ast.h src/parser.h src/value.h src/interpreter.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "=== Test 1: Odd/Even Checker ==="
	@./$(TARGET) tests/test1.js
	@echo ""
	@echo "=== Test 2: Triangle Pattern ==="
	@./$(TARGET) tests/test2.js
	@echo ""
	@echo "=== Test 3: Armstrong Number ==="
	@./$(TARGET) tests/test3.js
	@echo ""
	@echo "=== Test 4: Array Reverse ==="
	@./$(TARGET) tests/test4.js
	@echo ""
	@echo "=== Test 5: String Palindrome ==="
	@./$(TARGET) tests/test5.js
	@echo ""
	@echo "All tests completed."