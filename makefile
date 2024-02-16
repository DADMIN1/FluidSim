#CODEFILES=${./*.cpp}
CXX = g++-12
# DNDEBUG flag disables assert statements
SHAREDARGS = --std=c++20 -O2 -DNDEBUG
DEBUGARGS = --std=c++20 -Og -g

.PHONY: fluidsym
fluidsym:
	${CXX} ${SHAREDARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${SHAREDARGS} -o fluidsym ./*.o -lsfml-system -lsfml-graphics -lsfml-window
	./fluidsym


# TODO: stick 'dbg' on the object files compiled with debug files
.PHONY: debug
debug:
	${CXX} ${DEBUGARGS} -c ./*.cpp -lsfml-system -lsfml-graphics -lsfml-window
	${CXX} ${DEBUGARGS} -o fluidsym_dbg ./*.o -lsfml-system -lsfml-graphics -lsfml-window
#	./fluidsym_dbg


# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [makefile:24: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well
clean:
	@-rm --verbose fluidsym 2> /dev/null || true
	@-rm --verbose fluidsym_dbg 2> /dev/null || true
	@-rm --verbose ./*.o 2> /dev/null || true

