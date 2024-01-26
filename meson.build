project('libhashtable', 'c',
        default_options: 'c_std=c99',
        version: '0.6.6',
        license: 'MIT')

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-DXOPEN=600', language: 'c')

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')
