project('HiCR', [ 'c', 'cpp' ],
  version: '0.0.1',
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

hicrDeps = [
    dependency('hwloc', required: true),
    dependency('openmp', required: true),
    dependency('threads', required: true),
    dependency('boost',  modules : ['context'],  required : true)
   ]

hicrCppArgs = [ '-DHICR_ENABLE_BACKEND_SHARED_MEMORY', '-Wfatal-errors' ]

hicrIncludes = include_directories([
   '../include'
  ])
  
# Test build targets
if get_option('test')
  nomalloc = environment({'MALLOC_PERTURB_': '0'})
  subdir('tests')
endif