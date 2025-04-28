kernelsPath = ''

###### Compile Ascend kernels
if 'ascend' in enabledBackends
    fs = import('fs')
    # Check if ascend directory exists. If not, compile kernels
    if not fs.is_dir(meson.current_source_dir() + '/ascend')
        run_command('compile-ascend.sh', check: true)
    	message('Compiled Ascend kernels')
    else
        message('Use existing Ascend kernels')
    endif
    kernelsPath = meson.current_source_dir() + '/ascend'
endif

###### Set the kernelPath to opencl file
if 'opencl' in enabledBackends
	kernelsPath = meson.current_source_dir() + '/kernels.cl'
endif