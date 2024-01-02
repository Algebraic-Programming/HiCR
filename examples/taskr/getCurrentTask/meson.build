testSuite = [ 'examples', 'taskr', 'getCurrentTask' ]

getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep])

if get_option('buildTests')
test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite)
endif
