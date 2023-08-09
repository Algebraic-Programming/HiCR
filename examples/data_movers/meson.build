project('data_movers', [ 'c', 'cpp' ],
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
    #dependency('lpf', required: true),
   ]

taskr_include_dir = '../../../frontends/taskr/include'
hicr_include_dir = '../../../include'
boost_dep = dependency('boost',  modules : ['context'],  required : true)


#projectIncludes = include_directories([
#   'source', '/home/kdichev/.local/include'
#  ])

cppArgs = [
  '-ffast-math',
  '-Werror'
]

lpf_lib_dir = '/home/kdichev/.local/lib'
lpf_lib = 'lpf_core_univ_pthread_generic'
lpf_header_dir = '/home/kdichev/.local/include'
lpf_dep = declare_dependency(
    link_args : ['-L' + lpf_lib_dir, '-l' + lpf_lib],
    include_directories : include_directories(lpf_header_dir))

executable('test1',
  'source/test1.cpp',
  dependencies: [projectDeps, boost_dep, lpf_dep],
  #dependencies: [projectDeps, boost_dep],
  #build_rpath: [lpf_lib_dir],
  cpp_args: [ cppArgs, '-I' + taskr_include_dir, '-I' + hicr_include_dir, '-DHICR_ENABLE_BACKEND_PTHREADS' ],
)
