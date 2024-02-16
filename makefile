#CODEFILES=${./*.cpp}
CXX = g++-12
# DNDEBUG flag disables assert statements
#SHAREDARGS = --std=c++20 -O3 -DNDEBUG
SHAREDARGS = --std=c++20 -O3
DEBUGARGS = --std=c++20 -Og -g


fluidsym: *.cpp *.hpp makefile
	${CXX} ${SHAREDARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${SHAREDARGS} -o fluidsym ./*.o -lsfml-system -lsfml-graphics -lsfml-window
	./fluidsym


# TODO: stick 'dbg' on the object files compiled with debug files
fluidsym_dbg: *.cpp *.hpp makefile
	${CXX} ${DEBUGARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${DEBUGARGS} -o fluidsym_dbg ./*.o -lsfml-system -lsfml-graphics -lsfml-window
#	./fluidsym_dbg


# basically an alias for fluidsym_dbg
.PHONY: debug
debug: fluidsym_dbg
# otherwise, the debug-build will be rebuilt every time because it's '.PHONY'
# (even if it's not, it will still rebuild every time; it'll think it needs to create a file called 'debug')

# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [makefile:24: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well
.PHONY: clean
clean:
	@-rm --verbose fluidsym 2> /dev/null || true
	@-rm --verbose fluidsym_dbg 2> /dev/null || true
	@-rm --verbose ./*.o 2> /dev/null || true

