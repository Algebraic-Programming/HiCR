import os
import glob
import shutil 
import pathlib

def findModules(baseDir, docFileName):
  moduleList = []
  for subdir, dirs, files in os.walk(baseDir):
    for dir in dirs:
      folderPath = os.path.join(subdir, dir)
      if (os.path.isfile(os.path.join(subdir, dir, docFileName))):
        moduleList.append(os.path.join(subdir, dir).replace(os.path.commonprefix([baseDir, subdir ]), ''))
  return moduleList

def copyModuleFiles(srcDir, dstDir, docFileName, moduleList):
   for module in moduleList:
    srcFile = os.path.join(srcDir, module, docFileName)
    dstFolder = os.path.join(dstDir, module)
    dstFile = os.path.join(dstFolder, docFileName)
    pathlib.Path(dstFolder).mkdir(parents=True, exist_ok=True)

    # Copying README file
    shutil.copy(os.path.join(srcDir, module, docFileName), os.path.join(dstDir, module))
    
    # Copying all images
    for filename in glob.glob(os.path.join(srcDir, module, '*.png')): shutil.copy(filename, os.path.join(dstDir, module))

# Setting standard documentation file
docFileName = 'README.rst'

# Detecting backends

backendsSrcDir = '../../include/hicr/backends/'
backendsDstDir = 'builtin/backends/'
backendList = findModules(backendsSrcDir, docFileName)

# Copying documentation file into the backends folders
copyModuleFiles(backendsSrcDir, backendsDstDir, docFileName, backendList)

# Creating RST formatted lists
backendFormattedList = ""
for backend in backendList: backendFormattedList += ('   ' + os.path.join('backends/', backend, docFileName)  + '\n')

# Detecting frontends
frontendsSrcDir = '../../include/hicr/frontends/'
frontendsDstDir = 'builtin/frontends/'
frontendList = findModules(frontendsSrcDir, docFileName)

# Copying documentation file into the frontends folders
copyModuleFiles(frontendsSrcDir, frontendsDstDir, docFileName, frontendList)

# Creating RST formatted lists
frontendFormattedList = ""
for frontend in frontendList: frontendFormattedList += ('   ' + os.path.join('frontends/', frontend, docFileName) + '\n')

# Detecting examples
examplesSrcDir = '../../examples/'
examplesDstDir = 'introduction/examples/'
exampleList = findModules(examplesSrcDir, docFileName)

# Copying documentation file into the frontends folders
copyModuleFiles(examplesSrcDir, examplesDstDir, docFileName, exampleList)

# Creating RST formatted lists
exampleFormattedList = ""
for example in exampleList: exampleFormattedList += ('   ' + os.path.join('examples/', example, docFileName) + '\n')

#### Getting the contents of the backends file

with open('builtin/backends.rst.base', 'r') as indexFile:
    indexContent = indexFile.read()

# Replacing index content with the proper backend list

newIndexContent = indexContent.replace('[[Backends]]', backendFormattedList)

# Saving new index file

with open('builtin/backends.rst', 'w') as indexFile:
    indexFile.write(newIndexContent)

#### Getting the contents of the frontends file

with open('builtin/frontends.rst.base', 'r') as indexFile:
    indexContent = indexFile.read()

# Replacing index content with the proper frontend list

newIndexContent = indexContent.replace('[[Frontends]]', frontendFormattedList)

# Saving new index file

with open('builtin/frontends.rst', 'w') as indexFile:
    indexFile.write(newIndexContent)

#### Getting the contents of the examples file

with open('introduction/examples.rst.base', 'r') as indexFile:
    indexContent = indexFile.read()

# Replacing index content with the proper example lists

newIndexContent = indexContent.replace('[[Examples]]', exampleFormattedList)

# Saving new index file

with open('introduction/examples.rst', 'w') as indexFile:
    indexFile.write(newIndexContent)
