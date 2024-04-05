CXX = g++
CXXFLAGS = -std=c++11 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib
LDLIBS = -lSDL2

TARGET = fractal_explorer
SOURCES = fractal_explorer.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET)