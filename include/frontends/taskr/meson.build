# Define TaskR dependencies

taskrBuildDeps = [
    hicrBuildDep,
    dependency('threads', required: true)
   ]

taskrBuildCppArgs = [
 ]

taskrBuildIncludes = include_directories([
   '../../../include'
  ])
  
taskrBuildDep = declare_dependency(
    compile_args: taskrBuildCppArgs,
    dependencies:  taskrBuildDeps,
    include_directories: taskrBuildIncludes,
    )