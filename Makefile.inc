#
# Toplevel build file for libxc
#

# Generate a list of header files - not worried about non-dependency
HEADERPATS= $(patsubst %,%/*.h,$(SUBPROJS))
HEADERS=$(wildcard $(HEADERPATS))

# Generate two lists of sources - one for non-main source files and
# another for main source files
SRCPATS= $(patsubst %,%/*.c,$(SUBPROJS)) $(patsubst %,%/*.cpp,$(SUBPROJS))
ALLSOURCES=$(wildcard $(SRCPATS))
SOURCES=$(patsubst %/main.c,,$(ALLSOURCES))
SOURCEMAINS= $(filter %/main.c,$(ALLSOURCES))

# Generate two list of target objects - one for non-main object files and
# another for main object files. We generate one more list of files
# (see below for list of executables)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
MAINOBJECTS=$(patsubst %.c,%.o,$(SOURCEMAINS))

# Work out the platform we are on. At the moment we are only compiling for
# gnu platforms, testing on Linux and Windows/Mingw32-W64.
#
# Prior to this file being included the calling Makefile must execute the
# platform script, either:
#    PLATFORM?=$(shell ./platform_name.sh)
# OR
#    PLATFORM?=$(shell ./platform_name.bat)
#
# depending on which platform Makefile is being used.

OSTYPE="None"
OSTYPE_LINUX=$(findstring Linux,$(PLATFORM))
OSTYPE_FREEBSD=$(findstring FreeBSD,$(PLATFORM))
OSTYPE_WINDOWS=$(findstring Windows,$(PLATFORM))

MACHINETYPE="None"
PLATFORMINFO:=$(subst :, ,$(PLATFORM))
MACHINETYPE=$(word 3,$(PLATFORMINFO))

VERSION=$(shell $(CAT) VERSION)

ifeq ($(VERSION), "")
	$(error Version file does not exist)
endif

ifeq ($(OSTYPE_FREEBSD),FreeBSD)
	OSTYPE=FreeBSD
	PLATFORM=POSIX
endif

ifeq ($(OSTYPE_LINUX),Linux)
	OSTYPE=Linux
	PLATFORM=POSIX
endif

ifeq ($(OSTYPE_WINDOWS),Windows)
	OSTYPE=Windows
	PLATFORM=WINDOWS
endif


LIBNAME?=ERROR_NAME_NOT_SET
LIBFILE_NAME=lib$(LIBNAME)-$(VERSION)
LIBEXT_DLL=so
LIBEXT_STATIC=a
EXEEXT=elf

MAINPROGNAMES=$(patsubst %.o,%.$(EXEEXT),$(MAINOBJECTS))
MAINPROGS_D=$(patsubst %main.$(EXEEXT),%main-d.$(EXEEXT),$(MAINPROGNAMES))
MAINPROGS_R=$(patsubst %main.$(EXEEXT),%main-r.$(EXEEXT),$(MAINPROGNAMES))

# The above changes when we are on Windows
ifeq ($(OSTYPE), Windows)
LIBEXT_DLL=dll
LIBEXT_STATIC=a
EXEEXT=exe
endif

CURRENTDIR=$(PWD)

# Windows cmd.exe does not understand '/' directory separators, so we
# have to fix them
ifeq ($(OSTYPE), Windows)
	CURRENTDIR=$(subst \,/,$(shell cd))
endif


# The output directories
LIBDIR=$(CURRENTDIR)/lib
INCDIR=$(CURRENTDIR)/include
DOCDIR=$(CURRENTDIR)/docs
DOCS=html latex xml man
PREREQS=$(LIBDIR) $(INCDIR) $(DOCDIR)

MAKEFILE_INC=$(CURRENTDIR)/Makefile.inc

# ###################################################################
# From this point on everything assumes a working GCC. If you want to
# build with another compiler you'll probably have to write your own
# scripts around the compiler to get the information that is needed, and
# to set the correct compilation/linkage flags.
#

CC=gcc
CXX=g++
LD=gcc
LDXX=gcc
AR=ar

MYINCLUDEDIRS+=\
	-I$(CURRENTDIR)\
	-I$(MINGW_LOCATION)/opt/include\

MYDEFINES+=\
	-DOSTYPE=$(OSTYPE)\
	-DOSTYPE_$(OSTYPE)\
	-DPLATFORM=$(PLATFORM)\
	-DPLATFORM_$(PLATFORM)\
	-DMACHINE_TYPE=$(MACHINETYPE)\
	-DMACHINE_TYPE_$(MACHINETYPE)

# Sub-makes must always use += to add to variables, and not simply
# overwrite the existing variables.
CFLAGS+= -c -W -Wall -std=c99 \
	$(MYINCLUDEDIRS)\
	$(MYDEFINES)

CXXFLAGS+= -c -W -Wall -std=c++11\
	$(MYINCLUDEDIRS)\
	$(MYDEFINES)

LDEXTRAFLAGS+=-lpthread $(MYLIBS)

DEBUG_FLAG= -ggdb
RELEASE_FLAG= -O3
ARFLAGS= rcs

TARGET=$(shell gcc -dumpmachine)
OUTPUT_DIR=$(LIBDIR)/$(TARGET)
EXEDIR=$(OUTPUT_DIR)/testprogs

# The release library files
LIBFILE_DLL_R=$(OUTPUT_DIR)/$(LIBFILE_NAME).$(LIBEXT_DLL)
LIBFILE_STATIC_R=$(OUTPUT_DIR)/$(LIBFILE_NAME).$(LIBEXT_STATIC)

# The debug library files
LIBFILE_DLL_D=$(OUTPUT_DIR)/$(LIBFILE_NAME)d.$(LIBEXT_DLL)
LIBFILE_STATIC_D=$(OUTPUT_DIR)/$(LIBFILE_NAME)d.$(LIBEXT_STATIC)

# If nothing is specified we default to 'debug'
ifeq (,$(MAKECMDGOALS))
MAKECMDGOALS=debug
endif

# Because OUTPUT_LIBS is not available dynamically
ifneq (,$(findstring debug,$(MAKECMDGOALS)))
CFLAGS+=$(DEBUG_FLAG)
OUTPUT_LIB_DLL=$(LIBFILE_DLL_D)
OUTPUT_LIB_STATIC=$(LIBFILE_STATIC_D)
OUTPUT_LIBS=$(LIBFILE_DLL_D) $(LIBFILE_STATIC_D)
MAINPROGS=$(patsubst %main.$(EXEEXT),%main-d.$(EXEEXT),$(MAINPROGNAMES))
endif

ifneq (,$(findstring release,$(MAKECMDGOALS)))
CFLAGS+=$(RELEASE_FLAG)
OUTPUT_LIB_DLL=$(LIBFILE_DLL_R)
OUTPUT_LIB_STATIC=$(LIBFILE_STATIC_R)
OUTPUT_LIBS=$(LIBFILE_DLL_R) $(LIBFILE_STATIC_R)
MAINPROGS=$(patsubst %main.$(EXEEXT),%main-r.$(EXEEXT),$(MAINPROGNAMES))
endif

# Default target is the debug target
# debug:	CFLAGS+=$(DEBUG_FLAG)
# debug:	OUTPUT_LIB_DLL=$(LIBFILE_DLL_D)
# debug:	OUTPUT_LIB_STATIC=$(LIBFILE_STATIC_D)
# debug:	OUTPUT_LIBS=$(LIBFILE_DLL_D) $(LIBFILE_STATIC_D)
debug:	all

# Caller must explicitly set 'release' as a target
# release:	CFLAGS+=$(RELEASE_FLAG)
# release:	OUTPUT_LIB_DLL=$(LIBFILE_DLL_R)
# release:	OUTPUT_LIB_STATIC=$(LIBFILE_STATIC_R)
# release:	OUTPUT_LIBS=$(LIBFILE_DLL_R) $(LIBFILE_STATIC_R)
release:	all

.PHONY:	$(LIBFILE_DLL_R) $(LIBFILE_STATIC_R) $(LIBFILE_DLL_D) $(LIBFILE_STATIC_D) $(INCDIR)

show:
	@echo "CURRENTDIR:        $(CURRENTDIR)"
	@echo "PLATFORM:          $(PLATFORM)"
	@echo "OSTYPE:            $(OSTYPE)"
	@echo "PLATFORMINFO:      $(PLATFORMINFO)"
	@echo "MACHINETYPE:       $(MACHINETYPE)"
	@echo "LIBFILE_DLL_R:     $(LIBFILE_DLL_R)"
	@echo "LIBFILE_STATIC_R:  $(LIBFILE_STATIC_R)"
	@echo "LIBFILE_DLL_D:     $(LIBFILE_DLL_D)"
	@echo "LIBFILE_STATIC_D:  $(LIBFILE_STATIC_D)"
	@echo "EXEEXT:            $(EXEEXT)"
	@echo "CURRENTDIR:        $(CURRENTDIR)"
	@echo "MAKEFILE_INC:      $(MAKEFILE_INC)"
	@echo "CC:                $(CC)"
	@echo "CXX:               $(CXX)"
	@echo "CFLAGS:            $(CFLAGS)"
	@echo "CXXFLAGS:          $(CXXFLAGS)"
	@echo "LD:                $(LD)"
	@echo "LDFLAGS:           $(LDFLAGS)"
	@echo "LDEXTRAFLAGS:      $(LDEXTRAFLAGS)"
	@echo "DEBUG_FLAG:        $(DEBUG_FLAG)"
	@echo "RELEASE_FLAG:      $(RELEASE_FLAG)"
	@echo "TARGET:            $(TARGET)"
	@echo "OUTPUT_DIR:        $(OUTPUT_DIR)"
	@echo "OUTPUT_LIBS:       $(OUTPUT_LIBS)"
	@echo "HEADERS:           $(HEADERS)"
	@echo "SOURCES:           $(SOURCES)"
	@echo "SOURCEMAINS:       $(SOURCEMAINS)"
	@echo "OBJECTS:           $(OBJECTS)"
	@echo "MAINOBJECTS:       $(MAINOBJECTS)"
	@echo "MAINPROGS:         $(MAINPROGS)"


help: show
	@echo "debug     Build debug objects, libraries and executables."
	@echo "release   Build release objects, libraries and executables."
	@echo "clean     Remove object files only, not executables and libraries."
	@echo veryclean  Remove all compiled/linked files."

$(OUTPUT_DIR) $(EXEDIR):
	$(MKDIR) $(subst /,$(DS),$@)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $<

$(LIBFILE_DLL_D) $(LIBFILE_DLL_R): $(OUTPUT_DIR) all_objects
	$(LD) -shared $(LDFLAGS) -o $@ $(OBJECTS) $(LDEXTRAFLAGS)

$(LIBFILE_STATIC_D) $(LIBFILE_STATIC_R): $(OUTPUT_DIR) all_objects
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

%-r.$(EXEEXT): %.o $(OUTPUT_LIB_STATIC)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDEXTRAFLAGS)

%-d.$(EXEEXT): %.o $(OUTPUT_LIB_STATIC)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDEXTRAFLAGS)


# This rule is different between sh and cmd.exe
$(INCDIR) $(DOCDIR):
	-$(MKDIR) $(subst /,$(DS),$@)

all_objects: $(OBJECTS) $(MAINOBJECTS)

# This rule is different between sh and cmd.exe
clean:
	$(RM) $(subst /,$(DS),$(OBJECTS) $(MAINOBJECTS))

# This rule will not work under Windows
documentation:	$(DOCDIR)
	@echo No documentation rules are specified. Target 'documentation' is empty


# This rule is different between sh and cmd.exe
veryclean:	clean
	$(RM) $(subst /,$(DS), $(LIBFILE_DLL_D) $(LIBFILE_STATIC_D)\
		$(LIBFILE_DLL_R) $(LIBFILE_STATIC_R)\
		$(MAINPROGS_D) $(MAINPROGS_R)\
		$(INCDIR) $(DOCDIR))

# Run only after veryclean, debug, clean, release
namelibs:
	$(CP) $(LIBFILE_DLL_D) $(OUTPUT_DIR)/lib$(LIBNAME)d.$(LIBEXT_DLL)
	$(CP) $(LIBFILE_DLL_R) $(OUTPUT_DIR)/lib$(LIBNAME).$(LIBEXT_DLL)
	$(CP) $(LIBFILE_STATIC_D) $(OUTPUT_DIR)/lib$(LIBNAME)d.$(LIBEXT_STATIC)
	$(CP) $(LIBFILE_STATIC_R) $(OUTPUT_DIR)/lib$(LIBNAME).$(LIBEXT_STATIC)

