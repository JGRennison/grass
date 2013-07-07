#variables which can be pre-set (non-exhaustive list)
#ARCH: value for the switch: -march=
#GCC: path to g++
#debug: set to true to for a debug build
#list: set to true to enable listings
#map: set to true to enable linker map
#noexceptions: set to build without exceptions
#On windows only:
#x64: set to true to compile for x86_64/win64


SRC_DIRS := main core test

GENERIC_SRC = $(wildcard src/$1/*.cpp)
GENERIC_OBJ_DIR = objs/$1$(DIR_POSTFIX)
INCLUDE_main = main core
INCLUDE_test = test core
INCLUDE_core = core
GENERIC_CFLAGS = $(CFLAGS) $(CFLAGS_$1) $(foreach dir,$(value INCLUDE_$1),-iquote include/$(dir))
GENERIC_CXXFLAGS = $(CXXFLAGS) $(CXXFLAGS_$1)
GENERIC_OBJS = $(patsubst src/$1/%.cpp,$(call GENERIC_OBJ_DIR,$1)/%.o,$(call GENERIC_SRC,$1))
LIST_OBJS = $(foreach dir,$1,$(call GENERIC_OBJS,$(dir)))

OUTNAME:=grass
TESTOUTNAME=$(OUTNAME)-test
CFLAGS=-O3 -Wextra -Wall -Wno-unused-parameter
#-Wno-missing-braces -Wno-unused-parameter
CXXFLAGS:=-std=gnu++0x
GCC:=g++
LD:=ld
DIRS = $(foreach dir,$(SRC_DIRS),$(call GENERIC_OBJ_DIR,$(dir))) $(call GENERIC_OBJ_DIR,test)/pch

ifdef noexceptions
CXXFLAGS += -fno-exceptions
NXPOSTFIX:=_nx
DIR_POSTFIX:=$(DIR_POSTFIX)$(NXPOSTFIX)
OUTNAME:=$(OUTNAME)$(NXPOSTFIX)
endif

ifdef debug
CFLAGS=-g -Wextra -Wall -Wno-unused-parameter
#AFLAGS:=-Wl,-d,--export-all-symbols
DEBUGPOSTFIX:=_debug
DIR_POSTFIX:=$(DIR_POSTFIX)$(DEBUGPOSTFIX)
OUTNAME:=$(OUTNAME)$(DEBUGPOSTFIX)
endif

GCCMACHINE:=$(shell $(GCC) -dumpmachine)
ifeq (mingw, $(findstring mingw,$(GCCMACHINE)))
#WIN
PLATFORM:=WIN
AFLAGS+=-s -static -Llib
AFLAGS_main+=-mwindows -LC:/SourceCode/Libraries/wxWidgets2.8/lib/gcc_lib
GFLAGS=-mthreads
SUFFIX:=.exe
LIBS_main32=-lwxmsw28u_richtext -lwxmsw28u_aui -lwxbase28u_xml -lwxexpat -lwxmsw28u_html -lwxmsw28u_adv -lwxmsw28u_media -lwxmsw28u_core -lwxbase28u -lwxjpeg -lwxpng -lwxtiff -lwldap32 -lws2_32 -lgdi32 -lshell32 -lole32 -luuid -lcomdlg32 -lwinspool -lcomctl32 -loleaut32 -lwinmm
LIBS_main64=
GCC32=i686-w64-mingw32-g++
GCC64=x86_64-w64-mingw32-g++
CFLAGS_main=-isystem C:/SourceCode/Libraries/wxWidgets2.8/include
HDEPS:=
EXECPREFIX:=
PATHSEP:=\\
MKDIR:=mkdir

ifdef x64
SIZEPOSTFIX:=64
DIR_POSTFIX:=$(DIR_POSTFIX)$(SIZEPOSTFIX)
OUTNAME:=$(OUTNAME)$(SIZEPOSTFIX)
GCC:=$(GCC64)
LIBS_main:=$(LIBS_main64)
CFLAGS+=-mcx16
else
GCC:=$(GCC32)
LIBS_main:=$(LIBS_main32)
ARCH:=i686
endif

else
#UNIX
PLATFORM:=UNIX
LIBS:=-lpcre -lrt
LIBS_main:=`wx-config --libs`
CFLAGS_main:= `wx-config --cxxflags`
GCC_MAJOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f1)
GCC_MINOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f2)
ARCH:=$(shell test $(GCC_MAJOR) -gt 4 -o \( $(GCC_MAJOR) -eq 4 -a $(GCC_MINOR) -ge 2 \) && echo native)
EXECPREFIX:=./
PATHSEP:=/
MKDIR:=mkdir -p

wxconf:=$(shell wx-config --selected-config)
ifeq (gtk, $(findstring gtk,$(wxconf)))
LIBS_main+=`pkg-config --libs glib-2.0`
#MCFLAGS+=`pkg-config --cflags glib-2.0`
endif

endif

CFLAGS_test += -iquote $(call GENERIC_OBJ_DIR,test)/pch -Winvalid-pch

GCCVER:=$(shell $(GCC) -dumpversion)

ifeq (4.7.0, $(GCCVER))
$(error GCC 4.7.0 has a nasty bug in std::unordered_multimap, this will cause problems)
endif

ifdef list
CFLAGS+= -masm=intel -g -save-temps=obj -Wa,-msyntax=intel,-aghlms=$(@:.o=.lst)
endif

ifdef map
AFLAGS+=-Wl,-Map=$(OUTNAME).map
endif

ifneq ($(ARCH),)
CFLAGS += -march=$(ARCH)
endif

.SUFFIXES:

all: $(OUTNAME)$(SUFFIX)
ifndef noexceptions
all: $(TESTOUTNAME)$(SUFFIX)
endif
main: $(OUTNAME)$(SUFFIX)
test: $(TESTOUTNAME)$(SUFFIX)

$(call GENERIC_OBJS,test): $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch

OBJS:=$(call LIST_OBJS,main core)
$(OUTNAME)$(SUFFIX): $(OBJS)
	$(GCC) $(OBJS) -o $(OUTNAME)$(SUFFIX) $(LIBS) $(LIBS_main) $(AFLAGS) $(AFLAGS_main) $(GFLAGS)

TEST_OBJS:=$(call LIST_OBJS,test core)
$(TESTOUTNAME)$(SUFFIX): $(TEST_OBJS)
	$(GCC) $(TEST_OBJS) -o $(TESTOUTNAME)$(SUFFIX) $(LIBS) $(AFLAGS) $(AFLAGS_test) $(GFLAGS)
	$(EXECPREFIX)$(TESTOUTNAME)$(SUFFIX)

MAKEDEPS = -MMD -MP -MT '$@ $(@:.o=.d)'

define COMPILE_RULE
$$(call GENERIC_OBJ_DIR,$1)/%.o: src/$1/%.cpp | $$(call GENERIC_OBJ_DIR,$1) ; $$(GCC) -c $$< -o $$@ $$(call GENERIC_CFLAGS,$1) $$(call GENERIC_CXXFLAGS,$1) $$(GFLAGS) $$(MAKEDEPS)
endef

$(foreach DIR,$(SRC_DIRS),$(eval $(call COMPILE_RULE,$(DIR))))

$(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch: | $(call GENERIC_OBJ_DIR,test)/pch
	$(GCC) -c include/test/catch.hpp -o $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch $(call GENERIC_CFLAGS,test) $(CXXFLAGS) $(GFLAGS) $(MAKEDEPS)

$(DIRS):
	-$(MKDIR) $(subst /,$(PATHSEP),$@)

.PHONY: clean install uninstall all main test

ALL_OBJS:=$(call LIST_OBJS,$(SRC_DIRS))
-include $(ALL_OBJS:.o=.d)

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:.o=.ii) $(ALL_OBJS:.o=.lst) $(ALL_OBJS:.o=.d) $(ALL_OBJS:.o=.s) $(OUTNAME)$(SUFFIX) $(TESTOUTNAME)$(SUFFIX) $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch

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
