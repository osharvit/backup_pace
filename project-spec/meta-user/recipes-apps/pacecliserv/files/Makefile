PACECLISERV = pacecliserv

# Add any other object files to this list below
PACECLISERV_OBJS = client.o clientmgr.o connmgr.o h2emul.o main.o socket.o

all: $(PACECLISERV)

LDLIBS += -lpthread

$(PACECLISERV): $(PACECLISERV_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(PACECLISERV_OBJS) $(LDLIBS)

clean:
	-rm -f $(PACECLISERV) *.elf *.gdb *.o


