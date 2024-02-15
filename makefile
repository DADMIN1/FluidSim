#CODEFILES=${./*.cpp}

.PHONY: fluidsym
fluidsym:
	g++ -O2 -c ./*.cpp
	g++ -O2 -o fluidsym ./*.o -lsfml-system -lsfml-graphics -lsfml-window
	./fluidsym

clean:
	rm fluidsym
	rm ./*.o
