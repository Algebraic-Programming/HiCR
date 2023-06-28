project('hicr', [ 'cpp' ],
  version: '1.0.0',
  default_options : [
  'cpp_std=c++20',
  'buildtype=release',
  ]
)

projectDeps = [
    #dependency('lapacke', required: true),
   ]

projectIncludes = include_directories([
    'source/'
  ])

cppArgs = [
  
]

cppSource = [
  'source/hicr/runtime.cpp',
  'source/hicr/backends/openmp/openmp.cpp'
]

library('hicr',
  cppSource,
  include_directories: projectIncludes,
  dependencies: projectDeps,
  cpp_args: cppArgs
)

install_headers( [
				  'source/hicr/runtime.hpp',
				  'source/hicr/logger.hpp',
				 ], subdir : 'hicr')

install_headers( [
				  'source/hicr/backends/backend.hpp',
				 ], subdir : 'hicr/backends')

install_headers( [
				  'source/hicr/backends/openmp/openmp.hpp',
				 ], subdir : 'hicr/backends/openmp/openmp.hpp')