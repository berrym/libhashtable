project('libhashtable', 'c',
        default_options: 'c_std=c11',
        version: '0.7.0',
        license: 'MIT')

add_project_arguments('-D_XOPEN_SOURCE=600', language: 'c')
add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')

if get_option('examples')
  subdir('examples')
endif
