# HiCR MPI Backend Configuration

# Try to get MPICH, which is a special case
mpiDep = dependency('mpi', language: 'cpp', required: true)
    
# If also not found, then trigger a failure
if mpiDep.found() == false
  mpiDep = dependency('mpich', required: false)
endif

hicrMPIBackendDeps = [ mpiDep  ]
hicrMPIBackendDep = declare_dependency( 
    dependencies:  hicrMPIBackendDeps
 )    
    


