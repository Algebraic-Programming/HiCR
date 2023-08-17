function check()
{
 if [ ! $? -eq 0 ]; then 
   echo "Error building documentation."
   exit -1 
 fi
}

doxygen; check
cp -r doxygen source/doxygen; check
sphinx-build -M html source/ build/; check
