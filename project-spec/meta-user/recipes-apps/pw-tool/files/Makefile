APP = pw-tool

# Add any other object files to this list below
APP_OBJS = pw-tool.o axis-fifo-eth-loop.o protocol.o
# pl_interface.o hw_interface.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(LDFLAGS)  -lrt -lpthread -Wall -Wextra $(LDLIBS)
clean:
	rm -f $(APP) *.o

