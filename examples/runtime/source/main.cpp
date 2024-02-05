#include <frontends/runtime/runtime.hpp>

int main(int argc, char *argv[])
{
  HiCR::Runtime::initialize();
  
  HiCR::Runtime::finalize();

  return 0;
}
