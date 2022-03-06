LIB = paceipc

MAJOR = 1.0
MINOR = 1
VERSION = $(MAJOR).$(MINOR)

SRCS = ipclib.c protocol.c socket.c threads.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
CFLAGS += -I. -Werror -Wall -std=gnu99 -fPIC

all: build
build: lib$(LIB).so

lib$(LIB).so: lib$(LIB).so.$(VERSION)
	rm -f lib$(LIB).so.$(MAJOR) lib$(LIB).so
	ln -s lib$(LIB).so.$(VERSION) lib$(LIB).so.$(MAJOR)
	ln -s lib$(LIB).so.$(MAJOR) lib$(LIB).so

lib$(LIB).so.$(VERSION): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) -shared -Wl,-soname,lib$(LIB).so.$(MAJOR) $(OBJS) $(LDLIBS)

clean:
	rm -f *.o *.d *.so *.so.*

-include $(DEPS)