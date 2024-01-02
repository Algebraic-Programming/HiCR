# Define HiCR dependencies

# Parsing exception model
exceptionModel = get_option('exceptionModel')
if (exceptionModel == 'stdexception')
 hicrExceptionCppArgs = [ '-DHICR_EXCEPTION_USE_STDEXCEPTION' ]
endif
if (exceptionModel == 'abort')
 hicrExceptionCppArgs = [ '-DHICR_EXCEPTION_USE_ABORT' ]
endif

hicrBuildDeps = [
    dependency('boost',  modules : ['context'],  required : true)
   ]

hicrBuildCppArgs = [
 '-Wfatal-errors',
 '-Werror',
 hicrExceptionCppArgs
 ]

hicrBuildIncludes = include_directories([
   '../../include',
   '../../extern'
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
