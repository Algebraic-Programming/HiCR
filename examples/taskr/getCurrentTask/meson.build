testSuite = [ 'examples', 'taskr' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
    getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: hicrBuildDep)

    if get_option('buildTests')
    test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite)
    endif
endif
