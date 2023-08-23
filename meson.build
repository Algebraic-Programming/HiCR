project('HiCR', [ 'cpp' ],
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

# Get compiler
cpp = meson.get_compiler('cpp')

hicrDeps = [
    dependency('hwloc', required: true),
    dependency('openmp', required: true),
    dependency('threads', required: true),
    dependency('boost',  modules : ['context'],  required : true)
   ]

hicrCppArgs = [
 '-Wfatal-errors',
 '-Werror',
 ]

hicrIncludes = include_directories([
   '../include'
  ])
  
# If using coverage analysis
if get_option('b_coverage')
  gcov_args = [
    '-fno-inline',
    '-Wno-error=cpp',
    '-DENABLE_COVERAGE',
  ]
  if cpp.get_id().to_lower() == 'gcc'
    gcov_args += [
      '-fno-default-inline',
      '-fno-inline-small-functions',
    ]
  endif
  
  add_project_arguments(gcov_args, language: ['cpp'])
endif  

# Test build targets
if get_option('test')
  nomalloc = environment({'MALLOC_PERTURB_': '0'})
  subdir('tests')
endif