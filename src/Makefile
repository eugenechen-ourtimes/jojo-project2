.PHONY: clean
TARGET=server client
all: $(TARGET)
INCLUDE=-I../include
CFLAG=
CPPFLAG=-std=c++11#-g -pthread -Wall #-D PRINT_DEBUG
GCC=g++
BIN=../bin

CLIENT_OBJS=Client.o option.o SignUpHelper.o
SERVER_OBJS=Server.o option.o

%.o: %.c
	@gcc $< -c $(CFLAG) $(INCLUDE)
%.o: %.cpp
	@echo "making: " $@
	@g++ $< -c $(CPPFLAG) $(INCLUDE)

server: $(SERVER_OBJS)
	@echo "making: " $@
	@g++ -o $(BIN)/$@ $(CPPFLAG) $(INCLUDE) $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	@echo "making: " $@
	@g++ -o $(BIN)/$@ $(CPPFLAG) $(INCLUDE) $(CLIENT_OBJS)
clean:
	rm -f *.o
	@for bin in $(TARGET); do \
		echo "rm -f $$bin"; \
		rm -f $(BIN)/$$bin; \
	done
