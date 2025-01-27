subdir('memcpy')
subdir('kernel')
subdir('topology')
subdir('topologyRPC')

if 'channel' in enabledFrontends
 subdir('channels')
endif 

if 'deployer' in enabledFrontends and 'machineModel' in enabledFrontends
  subdir('deployer')
endif

if 'tasking' in enabledFrontends
  subdir('tasking')
endif

if 'objectStore' in enabledFrontends
  subdir('objectStore')
endif
