# Base configuration for all tests

# Define an extra dependency for tests that want to use mock facilities
hicrMockDep = declare_dependency(
  dependencies: [gmock_dep],
  include_directories: ([ 'include' ])
)

HiCRTestBuildDependencies = [ hicrBuildDep, hicrTestDep ]

subdir('frontends')
subdir('backends')
