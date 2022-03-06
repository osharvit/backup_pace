APP = ipcclientapp

# Add any other object files to this list below
APP_OBJS = main.o

all: build

build: $(APP)

LDLIBS += -lpaceipc -lpthread
CFLAGS += -I${includedir}/paceipc

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(LDFLAGS) $(LDLIBS)
clean:
	rm -f $(APP) *.o
