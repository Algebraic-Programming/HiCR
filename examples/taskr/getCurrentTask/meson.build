testSuite = [ 'examples', 'taskr', 'getCurrentTask' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ])

    if get_option('buildTests')
    test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite)
    endif
endif