
# Your library name - this must follow all the rules for filenames on your
# platform, when those filenames are being passed to gcc/ld
LIBNAME=rotsit

# All the subprojects that make up this library. Ideally each one of the
# subdirectories will have a main.c file that runs a test on that module.
#
SUBPROJS=\
	rotsit\
	pdate\
	eval\
	cliapp

# Your extra include directories, for headers that are not in the path.
MYINCLUDEDIRS+= -I$(HOME)/include

# Your extra libararies, for libraries that are not in the path.
MYLIBS+= -L$(HOME)/lib/$(TARGET) -lxc -lregex

#############################################################
# You should not need to modify anything below this comment #
#############################################################

MAKEPROGRAM_EXE=$(findstring exe,$(MAKE))
MAKEPROGRAM_MINGW=$(findstring mingw,$(MAKE))

# Remember that freebsd uses gmake/gnu-make, not plain make

ifneq ($(MAKEPROGRAM_EXE),)
	# We are running on Windows for certain - not sure if cygwin or not
	SHELL=cmd.exe /c
	MAKESHELL=cmd.exe /c
	PLATFORM=$(shell windows\platform_name.bat)
include windows/Makefile.mingw
endif

ifneq ($(MAKEPROGRAM_MINGW),)
	# We are running on Windows/Mingw
	SHELL=cmd.exe /c
	MAKESHELL=cmd.exe /c
	PLATFORM=$(shell windows\platform_name.bat)
include windows/Makefile.mingw
endif

# If neither of the above are true then we assume a posix platform
ifeq ($(PLATFORM),)
	PLATFORM=$(shell posix/platform_name.sh)
include posix/Makefile.posix
endif

