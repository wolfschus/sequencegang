CC       = gcc
CXX      = g++
CCFLAGS  = -Wall
CXXFLAGS = -Wall 
LDFLAGS  = -lSDL -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL_mixer -lrtmidi -lpthread -lfluidsynth -lsqlite3

TARGET  = sequencegang
OBJECTS = main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJECTS)

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm $(TARGET)
	rm $(OBJECTS)

setup:
	mkdir ~/.sequencegang
	cp -r db/* ~/.sequencegang

install:
	cp $(TARGET) /usr/local/bin/
	rm $(TARGET)
