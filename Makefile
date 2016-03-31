.PHONY : clean

CPPFLAGS=-fPIC -g --std=c++14
LDFLAGS=-shared 

SOURCES = SerializationSupport.cpp
HEADERS = SerializationMacros.hpp SerializationSupport.hpp
OBJECTS=$(SOURCES:.cpp=.o)

TARGET=mutils-serialization.so

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

