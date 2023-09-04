project('HiCR', [ 'cpp' ],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Including dependencies
subdir('include/hicr/')
subdir('include/taskr/')
subdir('include/hicr/backends/sharedMemory/pthreads')    
    
# Build example targets
subdir('examples')

# Build test targets
subdir('tests')

