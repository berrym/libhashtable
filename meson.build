project('libhashtable', 'c',
        default_options: 'c_std=c99',
        version: '0.1.0',
        license: 'MIT')

if host_machine.cpu_family() == 'x86'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'x86_64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
endif

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')
