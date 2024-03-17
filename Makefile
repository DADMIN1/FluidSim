#CODEFILES=${./*.cpp}
CXX = g++-12
# DNDEBUG flag disables assert statements
#SHAREDARGS = --std=c++20 -O3 -DNDEBUG
SHAREDARGS = --std=c++20 -O3
DEBUGARGS = --std=c++20 -Og -g

# TODO: implement 'debug' and 'assert' flags
# If the command-line args contained 'debug' as a target;
ifeq (debug, $(filter debug, $(MAKECMDGOALS)))
DEBUGFLAG = true
target_executable = fluidsym_dbg
else
DEBUGFLAG = false
target_executable = fluidsym
endif

#ifdef DEBUGFLAG

# cmdline-args passed to the program (space/comma seperated)
DEFAULT_RUNARGS := hello world three
# using quotes (either) here causes the whole line to be passed as a single argument
# on the command-line, you can put the whole thing in quotes(either), even the 'runargs=' part! (and it's still split correctly)

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


# if not defined ('runargs=...' wasn't specified on cmdline)
runargs ?= $(strip ${DEFAULT_RUNARGS})


# TODO: macro linker-flags
fluidsym: *.cpp *.hpp #Makefile
	${CXX} ${SHAREDARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${SHAREDARGS} -o fluidsym ./*.o -lsfml-system -lsfml-graphics -lsfml-window
#	./fluidsym


# TODO: stick 'dbg' on the object files compiled with debug files
fluidsym_dbg: *.cpp *.hpp Makefile
	${CXX} ${DEBUGARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${DEBUGARGS} -o fluidsym_dbg ./*.o -lsfml-system -lsfml-graphics -lsfml-window
#	./fluidsym_dbg



.PHONY: runargs
runargs:
	echo "runargs on cmdline"
	echo ${runargs}


.PHONY: PARSE_ARGS
PARSE_ARGS:
	echo "parse args"
	echo ${runargs}
#	newargs = ./PARSE_ARGS.bash 


#
# subdir example
# TODO: implement this
SUBDIRS = foo bar baz
.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	@mkdir --verbose --parents $@

# foo depends on baz
foo: baz
#
#


# the formatting/output options for 'time' are: '--portability' and '--verbose'; either is fine ('portability' preferred)
# not specifying either causes 'time' to print some garbage when running from a makefile (but not from normal terminal?!)
# '--append' does nothing without specifying an output file; I've included it by default in case a stray '-o' makes it onto the cmdline somehow
.PHONY: run
run: ${target_executable}
	echo ${runargs}
	./${target_executable} ${runargs}
#	time --portability --append ./${target_executable} ${runargs}
# example of garbage output: "0.00user 0.00system 0:00.00elapsed 91%CPU (0avgtext+0avgdata 2560maxresident)k0inputs+0outputs (0major+113minor)pagefaults 0swaps"


# basically an alias for fluidsym_dbg
.PHONY: debug
debug: fluidsym_dbg
# otherwise, the debug-build will be rebuilt every time because it's '.PHONY'
# (even if it's not, it will still rebuild every time; it'll think it needs to create a file called 'debug')


.PHONY: clean
clean:
	@-rm --verbose fluidsym 2> /dev/null || true
	@-rm --verbose fluidsym_dbg 2> /dev/null || true
	@-rm --verbose ./*.o 2> /dev/null || true
# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [Makefile:24: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well
