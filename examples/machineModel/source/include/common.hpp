#pragma once

#include <memory>
#include <fstream>
#include <sstream>

// Taken from https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c/116220#116220
inline std::string slurp(std::ifstream &in)
{
  std::ostringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

// Loads a string from a given file
inline bool loadStringFromFile(std::string &dst, const std::string fileName)
{
  std::ifstream fi(fileName);

  // If file not found or open, return false
  if (fi.good() == false) return false;

  // Reading entire file
  dst = slurp(fi);

  // Closing file
  fi.close();

  return true;
}