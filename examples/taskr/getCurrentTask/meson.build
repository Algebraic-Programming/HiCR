testSuite = [ 'examples', 'hicr', 'getCurrentTask' ]

getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSequentialBackendDep])
test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite)

