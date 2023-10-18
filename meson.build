project('HiCR', [ 'cpp', 'c'],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Including dependencies
subdir('include/hicr')
subdir('include/taskr')
subdir('include/hicr/backends/sequential')    
subdir('include/hicr/backends/sharedMemory')
subdir('include/hicr/backends/mpi')    
subdir('include/hicr/backends/lpf')    
    
# Build example targets
subdir('examples')

# Build test targets
#subdir('tests')

