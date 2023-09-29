project('HiCR', [ 'cpp' ],
  default_options : [
  'cpp_std=c++23',
  'buildtype=release',
  ]
)

# Including dependencies
subdir('include/hicr')
subdir('include/taskr')
subdir('include/hicr/backends/sequential')    
subdir('include/hicr/backends/sharedMemory')
subdir('include/hicr/backends/mpi')    
    
# Build example targets
subdir('examples')

# Build test targets
subdir('tests')

