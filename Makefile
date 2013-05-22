
SIGAR_PREFIX := $(shell realpath ~/src/altoros/moxi/repo20/install)

DEFAULT_INCLUDE := $(SIGAR_PREFIX)/include
DEFAULT_LIBS := $(SIGAR_PREFIX)/lib

CPPFLAGS ?=
CPPFLAGS += -I$(DEFAULT_INCLUDE)

LDFLAGS ?=
LDFLAGS += -L$(DEFAULT_LIBS) -Wl,-rpath=$(DEFAULT_LIBS) -lsigar

sigar_record: sigar_record.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f sigar_record sigar_record.o
	rm -f *.o
