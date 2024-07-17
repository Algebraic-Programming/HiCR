.DEFAULT: all
.PHONY: all doxygen doxysphinx sphinx sphinx-latex clean

all: doxygen doxysphinx sphinx

doxygen:
	doxygen 

doxysphinx:
	-doxysphinx build source/ build/html Doxyfile

sphinx:
	cd source; python3 ./generateModules.py; cd ..
	sphinx-build -M html source/ build -a

sphinx-latex:
	sphinx-build -M latex source/ build
	make -C build/latex clean
	make -C build/latex

clean:
	git clean -xdf .
