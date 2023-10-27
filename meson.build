project('HiCR', [ 'cpp', 'c'],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Including dependencies
subdir('include/hicr')
subdir('include/taskr')

# Getting selected backends
enabledBackends = get_option('backends')

# The sequential backend is the default for HiCR
subdir('include/hicr/backends/sequential')

# Including selected optional backends
if 'sharedMemory' in enabledBackends
  subdir('include/hicr/backends/sharedMemory')
endif

if 'mpi' in enabledBackends
  subdir('include/hicr/backends/mpi')
endif

if 'lpf' in enabledBackends
  subdir('include/hicr/backends/lpf')
endif

if 'ascend' in enabledBackends    
  subdir('include/hicr/backends/ascend')
endif    
    
# Build example targets
if get_option('buildExamples')
 subdir('examples')
endif

# Build test targets
if get_option('buildTests')
 subdir('tests')
endif
