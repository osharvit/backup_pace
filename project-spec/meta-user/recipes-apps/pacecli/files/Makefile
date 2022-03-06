PACECLI = pacecli

# Add any other object files to this list below
PACECLI_OBJS = 	clicpmgr.o	\
		cmd.o		\
		cmdmgr.o	\
		extcmdmgr.o	\
		file.o		\
		h2mgr.o		\
		helpmgr.o	\
		hwmgr.o		\
		json.o		\
		lexer.o		\
		luamgr.o	\
		luasockets.o	\
		main.o		\
		math.o		\
		rc_cmdmgr.o	\
		socket.o	\
		sync.o		\
		thread.o	\
		varmgr.o	\
		luaipcmgr.o	\
		ipcmgr.o	\
		cmdeditor.o	\
		path.o

CXXFLAGS += -Ilua/src
CXXFLAGS += -I${includedir}/paceipc

LDLIBS += -lpthread
LDLIBS += -Llua/libs -llua_pace -ldl -lrt -lpaceipc

all: $(PACECLI)

$(PACECLI): $(PACECLI_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(PACECLI_OBJS) $(LDLIBS)

clean:
	-rm -f $(PACECLI) *.elf *.gdb *.o


