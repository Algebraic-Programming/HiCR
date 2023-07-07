project('cholesky', [ 'c', 'cpp' ],
  version: '1.0.0',
  license: 'Apache-2.0',
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

projectDeps = [
    dependency('hwloc', required: true),
    dependency('openmp', required: true),
    dependency('threads', required: true),
    dependency('openblas', required: true),
    dependency('lapacke', required: true),
   ]

projectIncludes = include_directories([
   'extern',
   'source'
  ])

cppArgs = [
  '-ffast-math',
  '-Werror'
]

executable('cholesky_unblocked',
  [ 'source/main.cpp', 'source/variants/unblocked.cpp' ],
  include_directories: projectIncludes,
  dependencies: projectDeps,
  cpp_args: cppArgs
)

executable('cholesky_blocked_omp_tasks',
  [ 'source/main.cpp', 'source/variants/blocked_omp_tasks.cpp' ],
  include_directories: projectIncludes,
  dependencies: projectDeps,
  cpp_args: cppArgs
)

executable('cholesky_blocked_omp_threads',
  [ 'source/main.cpp', 'source/variants/blocked_omp_threads.cpp' ],
  include_directories: projectIncludes,
  dependencies: projectDeps,
  cpp_args: cppArgs
)

taskr_include_dir = '../../../frontends/taskr/include'
hicr_include_dir = '../../../include'
boost_dep = dependency('boost',  modules : ['context'],  required : true)

executable('cholesky_blocked_taskr_static',
  [ 'source/main.cpp', 'source/variants/blocked_taskr_static.cpp' ],
  include_directories: projectIncludes,
  dependencies: [ projectDeps, boost_dep ],
  cpp_args: [ cppArgs, '-I' + taskr_include_dir, '-I' + hicr_include_dir, '-DHICR_ENABLE_BACKEND_PTHREADS' ]
)

executable('cholesky_blocked_taskr_dynamic',
  [ 'source/main.cpp', 'source/variants/blocked_taskr_dynamic.cpp' ],
  include_directories: projectIncludes,
  dependencies: [ projectDeps, boost_dep],
  cpp_args: [ cppArgs, '-I' + taskr_include_dir, '-I' + hicr_include_dir, '-DHICR_ENABLE_BACKEND_PTHREADS'  ]
)