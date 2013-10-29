
CFLAGS += -fPIC -DPIC -ggdb -O1 -Wall -Wextra -Werror -Wsign-compare -Wundef -Wpointer-arith -Wvolatile-register-var -Wfatal-errors -DDP2P_VLC=0
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_THREAD_SAFE

LDFLAGS += -Wl,-no-undefined,-z,defs
LDFLAGS += -L/usr/lib -lpthread -lrt -lxml2 #-lstdc++ -lm -lc -lgcc_s

INCLUDES = -I. -Impd -Iutil -Ixml -I/usr/include -I/usr/include/libxml2
HEADERS = $(wildcard *.h mpd/*.h util/*.h xml/*.h)
#SOURCES = $(wildcard *.cpp mpd/*.cpp util/*.cpp xml/*.cpp)
SOURCES = Dashp2p.cpp DashHttp.cpp ThreadAdapter.cpp Utilities.cpp DebugAdapter.cpp Statistics.cpp MpdWrapper.cpp XmlAdapter.cpp \
          mpd/model.cpp mpd/ModelReader.cpp mpd/model_parser.cpp                                                                     \
          util/conversions.cpp                                                                                                       \
          xml/VLCDocumentAdapter.cpp

all: clean libdashp2p.so

clean:
	rm -f -- libdashp2p.so *.o mpd/*.o util/*.o xml/*.o

%.o : %.cpp $(HEADERS)
	g++ $(CFLAGS) $(INCLUDES) -c $< -o $@

libdashp2p.so: $(SOURCES:%.cpp=%.o)
	g++ -shared -o $@ $(CFLAGS) $^ $(LDFLAGS)
