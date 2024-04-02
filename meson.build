project('HiCR', [ 'cpp', 'c'],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Getting selected built-in backends
enabledBackends = get_option('backends')

# Including selected built-in backends
HiCRBackendDependencies = [ ]

if 'hwloc' in enabledBackends
  subdir('include/hicr/backends/host/hwloc')
endif

if 'pthreads' in enabledBackends
  subdir('include/hicr/backends/host/pthreads')
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

# Creating HiCR Core Configuration and Dependency
subdir('include/hicr/core')

# Build targets are only relevant if HiCR is being loaded as a subproject
if meson.is_subproject() == false

  # Build example targets
  if get_option('buildExamples')
  subdir('examples')
  endif

  # Build test targets
  if get_option('buildTests')
  subdir('tests')
  endif

endif