####### Create venv and install dependencies
fs = import('fs')
if not fs.is_dir(meson.current_source_dir() + '/.venv')
    run_command('create-venv.sh', check: true)
	message('Created venv')
else
    message('Use existing virtual environment')
endif

venvActivationPath = meson.current_source_dir() + '/.venv/bin/activate'

####### Train Neural Network
subdir('model')

####### Download MNIST test set
####### Defines: modelPath, testsetDir
subdir('dataset')
