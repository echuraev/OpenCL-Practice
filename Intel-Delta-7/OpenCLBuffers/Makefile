SOURCES=*.cpp
TARGET=OpenCLBuffers
OS := $(shell uname)
ifeq ($(OS),Darwin)
	CXX_FLAGS=-g -std=c++11 -framework OpenCL
else
	CXX_FLAGS=-g -std=c++11 -lOpenCL
endif

.PHONY: all

all: $(TARGET)

$(TARGET): $(SOURCES)
		$(CXX) $^ $(CXX_FLAGS) -o $@

.PHONY: clean

clean:
		rm -rvf $(TARGET) *.dSYM

