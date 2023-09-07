#include <cmath>

void work()
{
  __volatile__ double value = 2.0;
  for (size_t i = 0; i < 5000; i++)
    for (size_t j = 0; j < 5000; j++)
    {
      value = sqrt(value + i);
      value = value * value;
    }
}

