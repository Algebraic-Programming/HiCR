# HiCR MPI Backend Configuration

mpiDep = dependency('mpi', language: 'cpp', required: false)
    
# If a canonical version of MPI is not found, try to get MPICH, which is a special case
# If also not found, then trigger a failure
if mpiDep.found() == false
    mpiDep = dependency('mpich', required: true)
endif

hicrMPIBackendDeps = [ mpiDep  ]
hicrMPIBackendDep = declare_dependency( 
    dependencies:  hicrMPIBackendDeps
 )    
    


