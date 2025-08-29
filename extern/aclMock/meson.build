project(
  'acl hicr backend mock',
  ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
  ],
  version: '1.0.0',
)

# HiCR acl Mock Backend Configuration

backendIncludes = include_directories(['.'])

aclDependency = declare_dependency(
  sources: ['acl.cpp'],
  include_directories: backendIncludes,
)