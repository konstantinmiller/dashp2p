LD = ld
CC = cc

#VLC_PLUGIN_CFLAGS := $(shell pkg-config --cflags ../vlc/src/vlc-plugin.pc)
#VLC_PLUGIN_LIBS := $(shell pkg-config --libs ../vlc/src/vlc-plugin.pc)

override CFLAGS += -std=c++11 -fPIC -DPIC -ggdb -O1 -Wall -Wextra -Werror -Wsign-compare -Wundef -Wpointer-arith -Wvolatile-register-var -Wfatal-errors #-DDP2P_VLC=1
override CFLAGS += -DMODULE_STRING=\"dashp2p\" -D__PLUGIN__ -D__STDC_FORMAT_MACROS -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_THREAD_SAFE
override CFLAGS += -rdynamic

override LDFLAGS += -Wl,-no-undefined,-z,defs
override LDFLAGS += -Wl,-rpath,../vlc/src/.libs -L../vlc/src/.libs -L/usr/lib ../vlc/src/.libs/libvlccore.so -lpthread -lstdc++ -lm -lc -lgcc_s -lrt -lxml2

INCLUDES = -I. -Impd -Iutil -Ixml -I../vlc/include -I/usr/include -I/usr/include/libxml2
HEADERS = $(wildcard *.h mpd/*.h util/*.h xml/*.h)
SOURCES = $(wildcard *.cpp mpd/*.cpp util/*.cpp xml/*.cpp)

all: libdashp2p_plugin.so install

clean:
	rm -f -- libdashp2p_plugin.so *.o mpd/*.o util/*.o xml/*.o

%.o : %.cpp $(HEADERS)
	g++ $(CFLAGS) $(INCLUDES) -c $< -o $@

libdashp2p_plugin.so: $(SOURCES:%.cpp=%.o)
	g++ -shared -o $@ $(CFLAGS) $^ $(LDFLAGS)

install: libdashp2p_plugin.so
	cp libdashp2p_plugin.so ../vlc/modules/access/