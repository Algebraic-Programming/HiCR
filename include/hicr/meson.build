# Define HiCR dependencies

hicrBuildDeps = [
    dependency('boost',  modules : ['context'],  required : true)
   ]

hicrBuildCppArgs = [
 ]

hicrBuildIncludes = include_directories([
   '../../include'
  ])
  
hicrBuildDep = declare_dependency(
    compile_args: hicrBuildCppArgs,
    dependencies:  hicrBuildDeps,
    include_directories: hicrBuildIncludes,
    )

# Define HiCR standard test configuration and create a new dependency

hicrTestDeps = [
 dependency('gtest', required: true)
]

hicrTestCppFlags = [
 '-Wfatal-errors',
 '-Werror',
]

hicrTestLinkFlags = [
 '-lgtest_main',
 '-lgtest'
]

cpp = meson.get_compiler('cpp')
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
  
  hicrTestCppFlags += gcov_args
endif  

hicrTestDep = declare_dependency(
    compile_args: hicrTestCppFlags,
    link_args: hicrTestLinkFlags,
    dependencies:  hicrTestDeps,
    )
