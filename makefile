#variables which can be pre-set (non-exhaustive list)
#ARCH: value for the switch: -march=
#GCC: path to g++
#debug: set to true to for a debug build
#list: set to true to enable listings
#map: set to true to enable linker map
#On windows only:
#x64: set to true to compile for x86_64/win64


OBJS_SRC:=$(wildcard source/src/*.cpp)
TEST_SRC:=$(wildcard source/test/*.cpp)
OUTNAME:=grass
TESTOUTNAME:=$(OUTNAME)-test
CFLAGS=-O3 -Wextra -Wall -Wno-unused-parameter
#-Wno-missing-braces -Wno-unused-parameter
CXXFLAGS:=-std=gnu++0x
SRCCXXFLAGS:=-fno-exceptions
GCC:=g++
LD:=ld
OBJDIR:=objs/main
TESTOBJDIR:=objs/test
TSOBJDIR:=objs/src-test
DIRS=$(OBJDIR) $(TESTOBJDIR) $(TSOBJDIR)

ifdef debug
CFLAGS=-g -Wextra -Wall -Wno-unused-parameter
#AFLAGS:=-Wl,-d,--export-all-symbols
DEBUGPOSTFIX:=_debug
OBJDIR=$(OBJDIR)$(DEBUGPOSTFIX)
TESTOBJDIR=$(TESTOBJDIR)$(DEBUGPOSTFIX)
TSOBJDIR=$(TSOBJDIR)$(DEBUGPOSTFIX)
endif

GCCMACHINE:=$(shell $(GCC) -dumpmachine)
ifeq (mingw, $(findstring mingw,$(GCCMACHINE)))
#WIN
PLATFORM:=WIN
AFLAGS+=-s -static -Llib
SRCAFLAGS+=-mwindows -LC:/SourceCode/Libraries/wxWidgets2.8/lib/gcc_lib
GFLAGS=-mthreads
SUFFIX:=.exe
SRCLIBS32=-lwxmsw28u_richtext -lwxmsw28u_aui -lwxbase28u_xml -lwxexpat -lwxmsw28u_html -lwxmsw28u_adv -lwxmsw28u_media -lwxmsw28u_core -lwxbase28u -lwxjpeg -lwxpng -lwxtiff -lwldap32 -lws2_32 -lgdi32 -lshell32 -lole32 -luuid -lcomdlg32 -lwinspool -lcomctl32 -loleaut32 -lwinmm
SRCLIBS64=
GCC32=i686-w64-mingw32-g++
GCC64=x86_64-w64-mingw32-g++
SRCCFLAGS=-IC:/SourceCode/Libraries/wxWidgets2.8/include
HDEPS:=
EXECPREFIX:=
PATHSEP:=\\
MKDIR:=mkdir

ifdef x64
SIZEPOSTFIX:=64
OBJDIR+=$(SIZEPOSTFIX)
GCC:=$(GCC64)
SRCLIBS:=$(SRCLIBS64)
CFLAGS2:=-mcx16
PACKER:=mpress -s
else
GCC:=$(GCC32)
SRCLIBS:=$(SRCLIBS32)
ARCH:=i686
PACKER:=upx -9
endif

else
#UNIX
PLATFORM:=UNIX
LIBS:=-lpcre -lrt
SRCLIBS:=`wx-config --libs`
SRCCFLAGS:= `wx-config --cxxflags`
PACKER:=upx -9
#HDEPS:=
GCC_MAJOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f1)
GCC_MINOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f2)
ARCH:=$(shell test $(GCC_MAJOR) -gt 4 -o \( $(GCC_MAJOR) -eq 4 -a $(GCC_MINOR) -ge 2 \) && echo native)
EXECPREFIX:=./
PATHSEP:=/
MKDIR:=mkdir -p

wxconf:=$(shell wx-config --selected-config)
ifeq (gtk, $(findstring gtk,$(wxconf)))
LIBS+=`pkg-config --libs glib-2.0`
MCFLAGS+=`pkg-config --cflags glib-2.0`
endif

endif

OUTNAME:=$(OUTNAME)$(SIZEPOSTFIX)$(DEBUGPOSTFIX)

GCCVER:=$(shell $(GCC) -dumpversion)

ifeq (4.7.0, $(GCCVER))
$(error GCC 4.7.0 has a nasty bug in std::unordered_multimap, this will cause problems)
endif

TARGS:=$(OUTNAME)$(SUFFIX)

ifdef list
CFLAGS+= -masm=intel -g --save-temps -Wa,-msyntax=intel,-aghlms=$*.lst
endif

ifdef map
AFLAGS+=-Wl,-Map=$(OUTNAME).map
TARGS+=$(OUTNAME).map
endif

OBJS:=$(patsubst source/src/%.cpp,$(OBJDIR)/%.o,$(OBJS_SRC))
TEST_OBJS:=$(patsubst source/test/%.cpp,$(TESTOBJDIR)/%.o,$(TEST_SRC))
TS_OBJS:=$(TSOBJDIR)/track.o $(TSOBJDIR)/train.o $(TSOBJDIR)/trackcircuit.o $(TSOBJDIR)/traverse.o

ALL_OBJS:=$(OBJS) $(TEST_OBJS) $(TS_OBJS)

ifneq ($(ARCH),)
CFLAGS2 += -march=$(ARCH)
endif

.SUFFIXES:

all: $(OUTNAME)$(SUFFIX) $(TESTOUTNAME)$(SUFFIX)
main: $(OUTNAME)$(SUFFIX)
test: $(TESTOUTNAME)$(SUFFIX)

$(OUTNAME)$(SUFFIX): $(OBJS)
	$(GCC) $(OBJS) -o $(OUTNAME)$(SUFFIX) $(LIBS) $(SRCLIBS) $(AFLAGS) $(SRCAFLAGS) $(GFLAGS)

$(TESTOUTNAME)$(SUFFIX): $(TEST_OBJS)
	$(GCC) $(TEST_OBJS) $(TS_OBJS) -o $(TESTOUTNAME)$(SUFFIX) $(LIBS) $(AFLAGS) $(GFLAGS)
	$(EXECPREFIX)$(TESTOUTNAME)$(SUFFIX)

$(OBJDIR)/%.o: source/src/%.cpp
	$(GCC) -c $< -o $@ $(CFLAGS) $(SRCCFLAGS) $(CFLAGS2) $(CXXFLAGS) $(SRCCXXFLAGS) $(GFLAGS)

$(TESTOBJDIR)/%.o: source/test/%.cpp
	$(GCC) -c $< -o $@ $(CFLAGS) $(CFLAGS2) $(CXXFLAGS) $(GFLAGS)

$(TSOBJDIR)/%.o: source/src/%.cpp
	$(GCC) -c $< -o $@ $(CFLAGS) $(CFLAGS2) $(CXXFLAGS) $(GFLAGS)

$(ALL_OBJS): | $(DIRS)

$(DIRS):
	-$(MKDIR) $(subst /,$(PATHSEP),$@)

INC_TC:=source/include/trackcircuit.h
TRACTIONTYPE_INC:=source/include/tractiontype.h
INC_TRACK:=source/include/track.h $(INC_TC) $(TRACTIONTYPE_INC)
INC_TIMETABLE:=source/include/timetable.h
INC_TRAIN:=source/include/train.h $(INC_TRACK) $(INC_TIMETABLE)
INC_TRAVERSE:=source/include/traverse.h $(INC_TRACK)
INC_SIGNAL:=source/include/signal.h $(INC_TRACK)

DEPDIRS:=$(OBJDIR) $(TESTOBJDIR) $(TSOBJDIR)
$(addsuffix /track.o,$(DEPDIRS)): $(INC_TRACK)
$(addsuffix /train.o,$(DEPDIRS)): $(INC_TRAIN) source/include/util.h $(INC_TRAVERSE)
$(addsuffix /traverse.o,$(DEPDIRS)): $(INC_TRAVERSE)
$(addsuffix /signal.o,$(DEPDIRS)): $(INC_SIGNAL)

$(addsuffix /trackcircuit.o,$(DEPDIRS)): $(INC_TC)
$(addsuffix /cmdline.o,$(DEPDIRS)): source/include/cmdline.h source/include/SimpleOpt.h
$(OBJDIR)/main.o: source/include/main.h source/include/cmdline.h source/include/wxcommon.h
$(OBJDIR)/cmdline.o: source/include/wxcommon.h

$(TS_OBJS) $(OBJS): source/include/common.h

$(TESTOUTNAME)$(SUFFIX): $(TS_OBJS)

.PHONY: clean install uninstall all main test

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:.o=.ii) $(ALL_OBJS:.o=.lst) $(ALL_OBJS:.o=.s) $(OUTNAME)$(SUFFIX) $(OUTNAME)_debug$(SUFFIX) $(TESTOUTNAME)$(SUFFIX) $(TESTOUTNAME)_debug$(SUFFIX)

install:
ifeq "$(PLATFORM)" "WIN"
	@echo Install only supported on Unixy platforms
else
	cp $(OUTNAME)$(SUFFIX) /usr/bin/$(OUTNAME)$(SUFFIX)
endif

uninstall:
ifeq "$(PLATFORM)" "WIN"
	@echo Uninstall only supported on Unixy platforms
else
	rm /usr/bin/$(OUTNAME)$(SUFFIX)
endif
