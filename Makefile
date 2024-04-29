MAKEFLAGS += -j8
CXX := g++-12
LDFLAGS := -lsfml-system -lsfml-graphics -lsfml-window -lpthread

# If the command-line args contained 'debug' as a target;
ifeq (debug, $(filter debug, $(MAKECMDGOALS)))
DEBUGFLAG = true
target_executable = fluidsym_dbg
OBJECTFILE_DIR = build/objects_dbg
CXXFLAGS := -std=c++23 -g -Og -pipe -Wall -Wextra -Wpedantic -Wfatal-errors
# c++23 standard isn't required for anything; 20 works fine
# '-g' is preferrable to '-ggdb'?? '-g3' or '-ggdb3' for extra info (like macro definitions). Default level is 2
else
DEBUGFLAG = false
target_executable = fluidsym
OBJECTFILE_DIR = build/objects
#CXXFLAGS := -std=c++23 -O3 -march=native -mtune=native -pipe -Wall -Wextra -Wpedantic -DNDEBUG
CXXFLAGS := -std=c++23 -O1 -pipe -Wall -Wextra -Wpedantic -Wfatal-errors
# -fmax-errors=1  or  -Wfatal-errors ???
endif

# TODO: -fvisibility-ms-compat  (although manpage says -fvisibility=hidden is preferred??)
# TODO: profiling
# TODO: add a 'release' build with more optimization and '-DNDEBUG'
# DNDEBUG flag disables assert statements

CODEFILES := $(wildcard *.cpp)
OBJFILES := $(patsubst %.cpp,$(OBJECTFILE_DIR)/%.o, $(CODEFILES))
#DEPFILES = $(patsubst %.cpp, build/deps/%.d, $(CODEFILES))
DEPFILES := $(OBJFILES:.o=.d)

# cmdline-args passed to the program (space/comma seperated)
DEFAULT_RUNARGS := hello world three
# using quotes (either) here causes the whole line to be passed as a single argument
# on the command-line, you can put the whole thing in quotes(either), even the 'runargs=' part! (and it's still split correctly)

# if not defined ('runargs=...' wasn't specified on cmdline)
runargs ?= $(strip ${DEFAULT_RUNARGS})

# TODO: implement lenient argument formatting (strip and split-on: commas, spaces-only, both, commas-only)
# TODO: implement super-lenient command-line arguments (also look for '--args/arg/runargs/=' in MAKECMDGOALS; parse arglist from cmdline)
#	this allows us to pass arguments without quotes, and format/write things more naturally

# Makefiles seem to be lenient with spacing around equal-signs (passed inside quoted line); 'A=x y z', 'A= x y z', 'A =x y z'
#	...will all define 'A' ( Makefile variable) as the list: 'x y z', regardless of the whitespace present
# This is also true on the commandline, but only if 'runargs' and '=' are included within the quoted line


# unnecessary; if we use '=' it'll be set as a variable??
#ifeq ($ "args=", (findstring "args=", ${MAKECMDGOALS})

# Test this
# 
# Command := $(firstword $(MAKECMDGOALS))
# Arguments := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
# 
# hello:
#     @echo "Hello, "$(Arguments)"!";
#


.PHONY: default
default: ${target_executable}


# build directories
SUBDIRS := build/objects build/objects_dbg
.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	@mkdir --verbose --parents $@
# '--parents' also prevents errors if it already exists


${target_executable}: $(OBJFILES) | ${SUBDIRS}
	${CXX} ${CXXFLAGS} -o $@ ${OBJFILES} ${LDFLAGS}

# the subdirs are added as an 'order-only' prerequisite; it doesn't trigger rebuilding
# that's important because the builddir's MTIME is updated every time a file is compiled;
# which would then trigger rebuilds for every file every time.

# this Makefile is added as a prerequisite to trigger rebuilds whenever it's modified

$(OBJECTFILE_DIR)/%.o: %.cpp Makefile | ${SUBDIRS}
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@
# The -MMD and -MP flags together create dependency-files (.d)
# actually don't use '-MP'; it creates fake empty dependency rules for the '.hpp' files


.PHONY: runargs
runargs:
	echo "runargs on cmdline"
	echo ${runargs}


.PHONY: PARSE_ARGS
PARSE_ARGS:
	echo "parse args"
	echo ${runargs}
#	newargs = ./PARSE_ARGS.bash 


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


.PHONY: clean
clean:
	@-rm --verbose fluidsym 2> /dev/null || true
	@-rm --verbose fluidsym_dbg 2> /dev/null || true
	@-rm --verbose ./build/*/* 2> /dev/null || true
#	@-rm --verbose ./*.o 2> /dev/null || true
# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [Makefile:24: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well


# this has to be at the end of the file?
-include $(DEPFILES)
# Include the .d makefiles. The '-' at the front suppresses the errors of missing depfiles.
# Initially, all the '.d' files will be missing, and we don't want those errors to show up.
