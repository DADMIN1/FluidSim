MAKEFLAGS += -j8 --output-sync=target --warn-undefined-variables

# testing for specific gcc versions (specifically, testing if '--version' prints a non-empty string)
# stderr redirected otherwise it prints 'make: g++-13: No such file or directory'
ifneq ($(shell g++-13 --version 2>/dev/null),)
CXX := g++-13
else ifneq ($(shell g++-12 --version 2>/dev/null),)
CXX := g++-12
# if the 'CXX' variable is already set, use that
else ifdef CXX
CXX := ${CXX}
else  # otherwise fallback to default gcc
CXX := g++
endif

CXXFLAGS := -pipe -std=c++23 -fdiagnostics-color=always
LDFLAGS := -lsfml-system -lsfml-graphics -lsfml-window -lpthread
WARNFLAGS := -Wall -Wextra -Wpedantic -fmax-errors=1
# -fmax-errors=1  or  -Wfatal-errors ?
# -pedantic-errors  vs -Werror=pedantic ???

PROJECT_DIR := $(shell pwd)
# If the command-line args contained 'debug' as a target;
ifeq (debug, $(filter debug, $(MAKECMDGOALS)))
DEBUGFLAG = true
target_executable = fluidsym_dbg
OBJECTFILE_DIR = build/objects_dbg
CXXFLAGS += -g -Og
# '-g' is preferrable to '-ggdb'?? '-g3' or '-ggdb3' for extra info (like macro definitions). Default level is 2
else
DEBUGFLAG = false
target_executable = fluidsym
OBJECTFILE_DIR = build/objects
CXXFLAGS += -O3
endif

CXXFLAGS += -march=native -mtune=native
LTOFLAGS := -flto=auto -fuse-linker-plugin -fno-fat-lto-objects
# '-fno-fat-lto-objects': fat-LTO object-files have both the intermediate language and object code,
# which makes them usable for normal, non-LTO linking. Disabling it improves compile times and file sizes
# '-ffat-lto-objects' is the flag to enable it (which is default)
CXXFLAGS += ${LTOFLAGS}

#CXXFLAGS += -fsanitize=address  # super slow
# -fstack-check -fstack-protector
# TODO: sanitizer options
# -fanalyzer -fsanitize=style? -fstack-protector -fvtable-verify=std|preinit|none -fvtv
# -Wsuggest-final-types is more effective with link-time optimization

# TODO: -fvisibility-ms-compat  (although manpage says -fvisibility=hidden is preferred??)
# TODO: profiling
# TODO: add a 'release' build with more optimization and '-DNDEBUG'
# DNDEBUG flag disables assert statements

CODEFILES := $(wildcard *.cpp)
OBJFILES := $(patsubst %.cpp,$(OBJECTFILE_DIR)/%.o, $(CODEFILES))
#DEPFILES = $(patsubst %.cpp, build/deps/%.d, $(CODEFILES))
DEPFILES := $(OBJFILES:.o=.d)


IMGUI_DIR := ./imgui
IMGUI_SOURCES := $(wildcard ${IMGUI_DIR}/*.cpp)
IMGUI_SOURCES += $(wildcard ${IMGUI_DIR}/backends/*.cpp)
IMGUI_SOURCES += $(wildcard ${IMGUI_DIR}/sfml/*.cpp)
OBJECTFILE_DIR_IMGUI = build/objects_imgui
OBJFILES_IMGUI := $(patsubst ${IMGUI_DIR}/%.cpp,$(OBJECTFILE_DIR_IMGUI)/%.o, $(IMGUI_SOURCES))
DEPFILES_IMGUI := $(OBJFILES_IMGUI:.o=.d)
IMGUI_LDFLAGS := -lglfw -lGL -lvulkan

INCLUDE_FLAGS := -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMGUI_DIR)/sfml


PROFILING_DIR := profiling
PROFILING_BUILD_DIR := build/profiling
PROFILING_FLAGS := -fprofile-dir=${PROFILING_DIR} -fprofile-note=${PROFILING_DIR}


# build directories
SUBDIRS := build/objects build/objects_dbg ${PROFILING_BUILD_DIR} ${PROFILING_DIR}
SUBDIRS += build/objects_imgui build/objects_imgui/backends build/objects_imgui/sfml
.PHONY: subdirs
subdirs: $(SUBDIRS)
$(SUBDIRS):
	@mkdir --verbose --parents $@
# '--parents' also prevents errors if it already exists


# this does the same thing as the normal recipe for .cpp->.o files, except it also dumps the optimization-info
${PROFILING_DIR}/%.optinfo: %.cpp | ${SUBDIRS}
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $(patsubst %.cpp,$(OBJECTFILE_DIR)/%.o, $<) ${INCLUDE_FLAGS} ${WARNFLAGS} -fopt-info-all=$@
	@echo "dumped optimization info to: $@ \n"

# -fopt-info  # dumps optimization
# -fopt-info-[option]=[filename]  # all dumps are concatenated into filename, otherwise it just prints on stderr
# -fsave-optimization-record
FOPTDUMPS := $(patsubst %.cpp,$(PROFILING_DIR)/%.optinfo, $(CODEFILES))

# dumps optimization info for all files
.PHONY: foptdump
foptdump: ${FOPTDUMPS}
	@echo "finished writing optimization info for: $? \n"
# '$?' means all dependencies that needed updating


.DEFAULT_GOAL := ${target_executable}
${target_executable}: ${OBJFILES} libimgui.so | ${SUBDIRS}
	${CXX} -L${PROJECT_DIR} ${CXXFLAGS} ${OBJFILES} ${WARNFLAGS} -o $@ -limgui ${LDFLAGS} -Wl,-rpath,${PROJECT_DIR}
# the linking MUST be done in a seperate step, because the compiler doesn't recognize "rpath" option
# 'rpath' hardcodes the dynamic-library location into the binary; otherwise you have to set your 'DYNAMIC_LINK_PATH'(whatever it is) env-var when launching the program
# https://www.cprogramming.com/tutorial/shared-libraries-linux-gcc.html

# the subdirs are added as an 'order-only' prerequisite; it doesn't trigger rebuilding
# that's important because the builddir's MTIME is updated every time a file is compiled;
# which would then trigger rebuilds for every file every time.

# this Makefile is added as a prerequisite to trigger rebuilds whenever it's modified

$(OBJECTFILE_DIR)/%.o: %.cpp Makefile | ${SUBDIRS}
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@ ${INCLUDE_FLAGS} ${WARNFLAGS}
# The -MMD and -MP flags together create dependency-files (.d)
# actually don't use '-MP'; it creates fake empty dependency rules for the '.hpp' files

# compile all imgui object files with -fpic (for shared library)
$(OBJECTFILE_DIR_IMGUI)/%.o: $(IMGUI_DIR)/%.cpp | ${SUBDIRS}
	$(CXX) -fpic $(CXXFLAGS) -MMD -c $< -o $@ ${INCLUDE_FLAGS} ${WARNFLAGS}
# 'Makefile' should also be specified here, especially if you change CXXFLAGS.
# But it's too annoying when it gets rebuilt unnecessarily

# create the shared library for imgui
# the name MUST be in the form "lib_.so"; otherwise the compiler won't find it with the '-l' flag
libimgui.so: ${IMGUI_SOURCES} ${OBJFILES_IMGUI} | ${SUBDIRS}
	$(CXX) -fpic -shared ${CXXFLAGS} ${OBJFILES_IMGUI} ${WARNFLAGS} ${LDFLAGS} ${IMGUI_LDFLAGS} -o $@
# TODO: need a debug-version of library (otherwise debugger cannot step into it or set breakpoints)
# 	speaking of which, need to utilize the files under imgui/debugger

# the formatting/output options for 'time' are: '--portability' and '--verbose'; either is fine ('portability' preferred)
# not specifying either causes 'time' to print some garbage when running from a makefile (but not from normal terminal?!)
# '--append' does nothing without specifying an output file; I've included it by default in case a stray '-o' makes it onto the cmdline somehow
.PHONY: run
run: ${target_executable}
	echo ${runargs}
	./${target_executable} ${runargs}
#	time --portability --append ./${target_executable} ${runargs}
# example of garbage output: "0.00user 0.00system 0:00.00elapsed 91%CPU (0avgtext+0avgdata 2560maxresident)k0inputs+0outputs (0major+113minor)pagefaults 0swaps"


# this is required to allow 'debug' on the command line;
# otherwise, it complains: "make: *** No rule to make target 'debug'.  Stop."
.PHONY: debug
debug: fluidsym_dbg


# TODO: doesn't clean the folders for debug build
.PHONY: clean
clean:
	@-rm --verbose fluidsym 2> /dev/null || true
	@-rm --verbose fluidsym_dbg 2> /dev/null || true
	@-rm --verbose ${OBJFILES} 2> /dev/null || true
	@-rm --verbose ${DEPFILES} 2> /dev/null || true
# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [Makefile:24: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well

# wipes the imgui build-files and lib as well
.PHONY: reallyclean
reallyclean: clean
	@-rm --verbose libimgui.so 2> /dev/null || true
	@-rm --verbose ${OBJFILES_IMGUI} 2> /dev/null || true
	@-rm --verbose ${DEPFILES_IMGUI} 2> /dev/null || true


# this has to be at the end of the file?
-include $(DEPFILES)
-include $(DEPFILES_IMGUI)
# Include the .d makefiles. The '-' at the front suppresses the errors of missing depfiles.
# Initially, all the '.d' files will be missing, and we don't want those errors to show up.
