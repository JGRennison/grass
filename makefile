#Variables which can be pre-set (non-exhaustive list)

#ARCH: value for the switch: -march=
#GCC: path to g++
#debug: set to true for a debug build
#gprof: set to true for a gprof build
#san: set to address, thread, leak, undefined, etc. for a -fsanitize= enabled build
#list: set to true to enable listings
#map: set to true to enable linker map
#noexceptions: set to build without exceptions
#gcov: set to true to buil with gcov support (disables optimisations) (this also uses lcov 1.11-pre1 (1.118) or later if available)
#cross: set to true if building on Unix, but build target is Windows
#V: set to true to show full command lines

#On Unixy platforms only
#WXCFGFLAGS: additional arguments for wx-config

#On windows only:
#x64: set to true to compile for x86_64/win64

#Note that to build on or for Windows, the include/lib search paths will need to be edited below and/or
#a number of libs/includes will need to be placed/built in a corresponding location where gcc can find them.

SRC_DIRS := main core util test layout text_cmd draw/wx draw/mod
MAIN_DIRS := main core util layout text_cmd draw/wx draw/mod
TEST_DIRS := test core util
RES_DIRS := draw/res
MAIN_RES := draw/res

GENERIC_SRC = $(wildcard src/$1/*.cpp)
GENERIC_OBJ_DIR = objs/$1$(DIR_POSTFIX)
GENERIC_CFLAGS = $(CFLAGS) $(CFLAGS_$(subst /,_,$1)) -iquote include -iquote deps/include
GENERIC_CXXFLAGS = $(CXXFLAGS) $(CXXFLAGS_$(subst /,_,$1))
GENERIC_OBJS = $(patsubst src/$1/%.cpp,$(call GENERIC_OBJ_DIR,$1)/%.o,$(call GENERIC_SRC,$1))
LIST_OBJS = $(foreach dir,$1,$(call GENERIC_OBJS,$(dir)))
GENERIC_RESOBJS = $(patsubst src/$1/%,$(call GENERIC_OBJ_DIR,$1)/%.o,$(wildcard src/$1/*))
LIST_RESOBJS = $(foreach dir,$1,$(call GENERIC_RESOBJS,$(dir)))

OUTDIR:=bin/
OUTNAME:=grass
TESTOUTNAME=$(OUTNAME)-test
CFLAGS=-g -O3 -Wextra -Wall -Wno-unused-parameter
AFLAGS=-g
#-Wno-missing-braces -Wno-unused-parameter
CXXFLAGS:=-std=gnu++0x
GCC:=g++
LD:=ld
DIRS = $(foreach dir,$(SRC_DIRS),$(call GENERIC_OBJ_DIR,$(dir))) $(foreach dir,$(RES_DIRS),$(call GENERIC_OBJ_DIR,$(dir))) $(call GENERIC_OBJ_DIR,test)/pch

EXECPREFIX:=./
PATHSEP:=/
MKDIR:=mkdir -p

CFLAGS_main=$(WX_CFLAGS)
CFLAGS_draw_wx=$(WX_CFLAGS)
CFLAGS_test=-O0

ifdef noexceptions
CXXFLAGS += -fno-exceptions
NXPOSTFIX:=_nx
DIR_POSTFIX:=$(DIR_POSTFIX)$(NXPOSTFIX)
OUTNAME:=$(OUTNAME)$(NXPOSTFIX)
endif

ifdef debug
CFLAGS=-g -Wextra -Wall -Wno-unused-parameter
AFLAGS=-g
#AFLAGS:=-Wl,-d,--export-all-symbols
DEBUGPOSTFIX:=_debug
DIR_POSTFIX:=$(DIR_POSTFIX)$(DEBUGPOSTFIX)
OUTNAME:=$(OUTNAME)$(DEBUGPOSTFIX)
WXCFGFLAGS:=--debug=yes
endif

ifdef gcov
CFLAGS += -O0 -fprofile-arcs -ftest-coverage
AFLAGS += -fprofile-arcs -lgcov
GCOVPOSTFIX:=_gcov
DIR_POSTFIX:=$(DIR_POSTFIX)$(GCOVPOSTFIX)
OUTNAME:=$(OUTNAME)$(GCOVPOSTFIX)
TESTCOVDIR:=cov
DIRS += $(TESTCOVDIR)
GCOVTESTNAME = $(subst -,_,$(TESTOUTNAME))
endif

ifdef gprof
CFLAGS += -pg
AFLAGS += -pg
GPROFPOSTFIX:=_gprof
DIR_POSTFIX:=$(DIR_POSTFIX)$(GPROFPOSTFIX)
OUTNAME:=$(OUTNAME)$(GPROFPOSTFIX)
endif

ifdef san
ifeq ($(san), thread)
CFLAGS += -fPIE -pie
AFLAGS += -fPIE -pie
endif
CFLAGS += -g -fsanitize=$(san) -fno-omit-frame-pointer
AFLAGS += -g -fsanitize=$(san)
SANPOSTFIX:=_san_$(san)
DIR_POSTFIX:=$(DIR_POSTFIX)$(SANPOSTFIX)
OUTNAME:=$(OUTNAME)$(SANPOSTFIX)
endif

GCCMACHINE:=$(shell $(GCC) -dumpmachine)
ifeq (mingw, $(findstring mingw,$(GCCMACHINE)))
#WIN
PLATFORM:=WIN
AFLAGS+=-s -static -Llib
AFLAGS_main+=-mwindows -Lwxlib
GFLAGS=-mthreads
SUFFIX:=.exe
LIBS_main32=-lwxmsw28u_richtext -lwxmsw28u_aui -lwxbase28u_xml -lwxexpat -lwxmsw28u_html -lwxmsw28u_adv -lwxmsw28u_media -lwxmsw28u_core -lwxbase28u -lwxjpeg -lwxpng -lwxtiff -lwldap32 -lws2_32 -lgdi32 -lshell32 -lole32 -luuid -lcomdlg32 -lwinspool -lcomctl32 -loleaut32 -lwinmm
LIBS_main64=
GCC32=i686-w64-mingw32-g++
GCC64=x86_64-w64-mingw32-g++
WX_CFLAGS+=-isystem wxinclude
HDEPS:=
ifndef cross
EXECPREFIX:=
PATHSEP:=\\
MKDIR:=mkdir
HOST:=WIN
endif

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
LIBS:=-lrt
LIBS_main:=`wx-config --libs $(WXCFGFLAGS)`
WX_CFLAGS+=$(patsubst -I/%,-isystem /%,$(shell wx-config --cxxflags $(WXCFGFLAGS)))
GCC_MAJOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f1)
GCC_MINOR:=$(shell $(GCC) -dumpversion | cut -d'.' -f2)
CFLAGS+=$(shell [ -t 0 ] && [ $(GCC_MAJOR) -gt 4 -o \( $(GCC_MAJOR) -eq 4 -a $(GCC_MINOR) -ge 9 \) ] && echo -fdiagnostics-color)

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

ifdef V
define EXEC
	$1
endef
else
ifeq ($HOST, WIN)
define EXEC
	@$1
endef
else
define EXEC
	@$1 ; rv=$$? ; [ $${rv} -ne 0 ] && echo 'Failed: $(subst ','"'"',$(1))' ; exit $${rv}
endef
endif
endif

.SUFFIXES:

FULLOUTNAME:=$(OUTDIR)$(OUTNAME)$(SUFFIX)
FULLTESTOUTNAME:=$(OUTDIR)$(TESTOUTNAME)$(SUFFIX)

all: $(FULLOUTNAME)
ifndef noexceptions
all: $(FULLTESTOUTNAME)
endif
main: $(FULLOUTNAME)
test: $(FULLTESTOUTNAME)

$(call GENERIC_OBJS,test): $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch

OBJS:=$(call LIST_OBJS,$(MAIN_DIRS)) $(call LIST_RESOBJS,$(MAIN_RES))
#This is to avoid unpleasant side-effects of over-writing executable in-place if it is currently running
$(FULLOUTNAME): $(OBJS) | $(OUTDIR)
	@echo '    Link       $(FULLOUTNAME)'
ifeq "$(HOST)" "WIN"
	$(call EXEC,$(GCC) $(OBJS) -o $(FULLOUTNAME) $(LIBS) $(LIBS_main) $(AFLAGS) $(AFLAGS_main) $(GFLAGS))
else
	$(call EXEC,$(GCC) $(OBJS) -o $(FULLOUTNAME).tmp $(LIBS) $(LIBS_main) $(AFLAGS) $(AFLAGS_main) $(GFLAGS))
	$(call EXEC,rm -f $(FULLOUTNAME))
	$(call EXEC,mv $(FULLOUTNAME).tmp $(FULLOUTNAME))
endif

TEST_OBJS:=$(call LIST_OBJS,$(TEST_DIRS)) $(call LIST_RESOBJS,$(TEST_RES))
$(FULLTESTOUTNAME): $(TEST_OBJS) | $(OUTDIR) $(TESTCOVDIR)
	@echo '    Link       $(FULLTESTOUTNAME)'
	$(call EXEC,$(GCC) $(TEST_OBJS) -o $(FULLTESTOUTNAME) $(LIBS) $(AFLAGS) $(AFLAGS_test) $(GFLAGS))
	@echo '    Test       $(FULLTESTOUTNAME)'
ifdef gcov
	-$(call EXEC,lcov -q -d . -z)
endif
	$(call EXEC,$(EXECPREFIX)$(FULLTESTOUTNAME))
ifdef gcov
	-$(call EXEC,lcov --no-external -q -b . -d . -c -t $(GCOVTESTNAME) -o $(TESTCOVDIR)/$(TESTOUTNAME).test)
	-$(call EXEC,lcov -q -r $(TESTCOVDIR)/$(TESTOUTNAME).test 'deps/**/*' -o $(TESTCOVDIR)/$(TESTOUTNAME).test)
	-$(call EXEC,genhtml -q --num-spaces 4 --legend --demangle-cpp $(TESTCOVDIR)/$(TESTOUTNAME).test -o $(TESTCOVDIR)/$(TESTOUTNAME))
endif

MAKEDEPS = -MMD -MP -MT '$@ $(@:.o=.d)'

define COMPILE_RULE
$$(call GENERIC_OBJ_DIR,$1)/%.o: src/$1/%.cpp | $$(call GENERIC_OBJ_DIR,$1)
	@echo '    g++        $$<'
	$$(call EXEC,$$(GCC) -c $$< -o $$@ $$(call GENERIC_CFLAGS,$1) $$(call GENERIC_CXXFLAGS,$1) $$(GFLAGS) $$(MAKEDEPS))
endef


RES_OBJCOPY = objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents $@ $@
ifeq "$(PLATFORM)" "WIN"
RES_LINK = $(GCC) -Wl,-r -Wl,-b,binary $< -o $@ -nostdlib
else
RES_LINK = $(LD) -r -b binary $< -o $@
endif

define RES_RULE
$$(call GENERIC_OBJ_DIR,$1)/%.o: src/$1/% | $$(call GENERIC_OBJ_DIR,$1)
	@echo '    res        $$<'
	$$(call EXEC,$$(RES_LINK))
	$$(call EXEC,$$(RES_OBJCOPY))
endef

$(foreach DIR,$(SRC_DIRS),$(eval $(call COMPILE_RULE,$(DIR))))

$(foreach DIR,$(RES_DIRS),$(eval $(call RES_RULE,$(DIR))))

$(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch: deps/include/test/catch.hpp | $(call GENERIC_OBJ_DIR,test)/pch
	@echo '    g++ (pch)  $<' ;
	$(call EXEC,$(GCC) -c deps/include/test/catch.hpp -o $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch $(call GENERIC_CFLAGS,test) $(CXXFLAGS) $(GFLAGS) $(MAKEDEPS))

$(DIRS) $(OUTDIR):
	-$(call EXEC,$(MKDIR) $(subst /,$(PATHSEP),$@))

.PHONY: clean install uninstall all main test

ALL_OBJS:=$(call LIST_OBJS,$(SRC_DIRS))
-include $(ALL_OBJS:.o=.d)

clean:
	@echo '    Clean all'
	$(call EXEC,rm -f $(ALL_OBJS) $(ALL_OBJS:.o=.ii) $(ALL_OBJS:.o=.lst) $(ALL_OBJS:.o=.d) $(ALL_OBJS:.o=.s) $(FULLOUTNAME) $(FULLOUTNAME).tmp $(FULLTESTOUTNAME) $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.gch $(call GENERIC_OBJ_DIR,test)/pch/catch.hpp.d)
	$(call EXEC,rm -f $(call LIST_RESOBJS,$(RES_DIRS)) $(subst .o,.d,$(call LIST_RESOBJS,$(RES_DIRS))))
ifdef gcov
	$(call EXEC,rm -f $(ALL_OBJS:.o=.gcno) $(ALL_OBJS:.o=.gcda))
	$(call EXEC,rm -rf $(TESTCOVDIR))
endif

install:
ifeq "$(PLATFORM)" "WIN"
	@echo Install only supported on Unixy platforms
else
	@echo '    Install to /usr/local/bin/$(OUTNAME)$(SUFFIX)'
	$(call EXEC,cp --remove-destination $(FULLOUTNAME) /usr/local/bin/$(OUTNAME)$(SUFFIX))
endif

uninstall:
ifeq "$(PLATFORM)" "WIN"
	@echo Uninstall only supported on Unixy platforms
else
	@echo '    Delete from /usr/local/bin/$(OUTNAME)$(SUFFIX)'
	$(call EXEC,rm /usr/local/bin/$(OUTNAME)$(SUFFIX))
endif
