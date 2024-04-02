# HiCR MPI Backend Configuration

mpiDep = dependency('mpi', language: 'cpp', required: true)
    
# If also not found, then trigger a failure
if mpiDep.found() == false
  mpiDep = dependency('mpich', required: false)
endif

mpirunExecutable = find_program('mpirun', '/usr/bin/mpirun', '/usr/local/bin/mpirun', required : true)

backendIncludes = include_directories([
])

backendDependencies = [
    mpiDep
]
    
backendCppArgs = [
   '-D_HICR_USE_MPI_BACKEND_'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    
