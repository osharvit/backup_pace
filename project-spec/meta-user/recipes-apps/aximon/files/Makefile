APP = aximon

# Add any other object files to this list below
APP_OBJS = main.o xaxipmon.o 
# pl_interface.o hw_interface.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(LDFLAGS)  -lrt -lpthread -Wall -Wextra $(LDLIBS)
clean:
	rm -f $(APP) *.o

