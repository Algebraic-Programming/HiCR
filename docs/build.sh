function check()
{
 if [ ! $? -eq 0 ]; then 
   echo "Error building documentation."
   exit -1 
 fi
}

# Clean any generated source
rm -rf build
rm -rf source/doxygen

# Regenerating documentation
doxygen; check
doxysphinx build source/ build source/doxygen/html/; check
sphinx-build -M html source/ build/; check
