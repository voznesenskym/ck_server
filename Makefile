CC=g++
CFLAGS=-c -Wall
LDFLAGS=-lpthread -lsodium -lzmq -lczmq -lcurve
SOURCES=main.cpp vz_rpcbase.cpp vz_server.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=vzserver

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -rf *o vzserver