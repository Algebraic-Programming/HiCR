testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
    getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: hicrBuildDep)

    if get_option('buildTests')
    test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite)
    endif
endif
