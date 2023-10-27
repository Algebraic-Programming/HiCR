projectIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include',
   'source'
  ])

cppArgs = [
  '-march=armv8.2-a+crypto+fp16',
  '-mcpu=tsv110',
  '-ffast-math',
  '-Werror',
  # '-ferror-limit=1',
]
  
linkArgs = [
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
  '-lascendcl',
]

executable('add_operator',
  'source/main.cpp',
  include_directories: projectIncludes,
  link_args: linkArgs,   
  cpp_args: cppArgs
)