project('abc', [ 'c', 'cpp' ],
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
   ]

taskr_include_dir = '../../../frontends/taskr/include'
hicr_include_dir = '../../../include'
boost_dep = dependency('boost',  modules : ['context'],  required : true)


projectIncludes = include_directories([
   'source'
  ])

cppArgs = [
  '-ffast-math',
  '-Werror'
]

executable('abc',
  [ 'source/main.cpp'],
  include_directories: projectIncludes,
  dependencies: [ projectDeps, boost_dep ],
  cpp_args: [ cppArgs, '-I' + taskr_include_dir, '-I' + hicr_include_dir, '-DHICR_ENABLE_BACKEND_SHARED_MEMORY', '-Wfatal-errors' ]
)