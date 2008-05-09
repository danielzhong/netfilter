CPP      = g++
CPPFLAGS = -funsigned-char -Wall -Werror -Wformat -I.
LDFLAGS  = -lnfnetlink -lgflags -lnetfilter_conntrack -lnetfilter_queue
OUT      = url-filter

ifdef DEBUG
  CPPFLAGS += -g
else
  CPPFLAGS += -O2 -pipe -Wuninitialized
endif

# Base rules.
all: base $(OUT)

clean:
	-rm -f $(OUT)
	-rm -f objs/*.o *~ .depend

base: objs/atomicops.o objs/logging.o objs/util.o

objs/atomicops.o: base/atomicops-internals-x86.cc base/atomicops-internals-x86.h base/atomicops.h
	$(CPP) $(CPPFLAGS) -c -o $@ base/atomicops-internals-x86.cc

objs/logging.o: base/logging.cc base/logging.h
	$(CPP) $(CPPFLAGS) -c -o $@ base/logging.cc

objs/util.o: base/util.cc base/util.h
	$(CPP) $(CPPFLAGS) -c -o $@ base/util.cc

# Project build rules.
objs/conntrack.o: conntrack.cc conntrack.h
	$(CPP) $(CPPFLAGS) -c -o $@ conntrack.cc

objs/packet.o: packet.cc packet.h
	$(CPP) $(CPPFLAGS) -c -o $@ packet.cc

objs/queue.o: queue.cc queue.h
	$(CPP) $(CPPFLAGS) -c -o $@ queue.cc

url-filter: objs/conntrack.o objs/packet.o objs/queue.o objs/atomicops.o objs/logging.o objs/util.o
