SI5518 = si5518

# Add any other object files to this list below
SI5518_OBJS = cmdmgr.o file.o main.o spimgr.o

all: $(SI5518)

LDLIBS += -lpthread

$(SI5518): $(SI5518_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(SI5518_OBJS) $(LDLIBS)

clean:
	-rm -f $(SI5518) *.elf *.gdb *.o


