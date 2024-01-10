project('HiCR', [ 'cpp', 'c'],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Including dependencies
subdir('include/hicr')
subdir('include/frontends/taskr')

# Getting selected backends
enabledBackends = get_option('backends')

# Including selected optional backends

if 'hwloc' in enabledBackends
  subdir('include/backends/host/hwloc')
endif

if 'pthreads' in enabledBackends
  subdir('include/backends/host/pthreads')
endif

if 'mpi' in enabledBackends
  subdir('include/backends/mpi')
endif

if 'lpf' in enabledBackends
  subdir('include/backends/lpf')
endif

if 'ascend' in enabledBackends    
  subdir('include/backends/ascend')
endif    
    
# Build example targets
if get_option('buildExamples')
 subdir('examples')
endif

# Build test targets
if get_option('buildTests')
 subdir('tests')
endif
