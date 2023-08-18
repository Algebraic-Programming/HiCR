.PHONY: all clean

all: 
	doxygen 
	doxysphinx build source/ build source/doxygen/html/
	sphinx-build -M html source/ build
clean:
	rm -rf build
	rm -rf source/doxygen

