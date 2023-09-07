.DEFAULT: all
.PHONY: all doxygen doxysphinx sphinx clean

all: doxygen doxysphinx sphinx

doxygen:
	doxygen 

doxysphinx:
	-doxysphinx build source/ build/html Doxyfile

sphinx:
	sphinx-build -M html source/ build

clean:
	git clean -xdf .
