kernelsPath = ''

###### Compile acl kernels
if 'acl' in enabledBackends
    fs = import('fs')
    # Check if acl directory exists. If not, compile kernels
    if not fs.is_dir(meson.current_source_dir() + '/acl')
        run_command('compile-acl.sh', check: true)
    	message('Compiled acl kernels')
    else
        message('Use existing acl kernels')
    endif
    kernelsPath = meson.current_source_dir() + '/acl'
endif

###### Set the kernelPath to opencl file
if 'opencl' in enabledBackends
	kernelsPath = meson.current_source_dir() + '/kernels.cl'
endif