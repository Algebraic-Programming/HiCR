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

# Creating HiCR Configuration and Dependency
subdir('include/hicr')

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