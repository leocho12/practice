CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
LIBS     = -lncurses
TARGET   = tetris
SRC      = tetris.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)
