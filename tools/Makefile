PBRT_TOOLS = exrassemble exravg exrtotiff tifftoexr ply2lux samplepat

ARCH = $(shell uname)

LUXDIR=..
LUXLIB=$(LUXDIR)/core/liblux.a
EXRDIR=../OpenEXR
OPT=-g
CXXFLAGS = $(OPT) -I/sw/include -I$(EXRDIR)/include -I$(LUXDIR)/core

ifeq ($(ARCH),Darwin)
LDFLAGS = -L$(EXRDIR)/lib-osx -L/sw/lib
LDFLAGS += -Wl,-all_load $(LUXDIR)/core/liblux.a 
CXXFLAGS += -Wno-long-double
else
LDFLAGS = -L$(EXRDIR)/lib-linux
LDFLAGS += -Wl,--export-dynamic -Wl,-whole-archive $(LUXLIB) -Wl,-no-whole-archive 
endif

LDFLAGS += -lIlmImf -lImath -lIex -lHalf -lz -ltiff -ldl

default: $(LUX_TOOLS)

%: %.cpp $(LUXLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

ply2lux: ply.h ply.c ply2lux.c
	$(CC) $(OPT) -I. -o ply2lux ply2lux.c ply.c -lm
clean:
	rm -f $(LUX_TOOLS) *.o *~
