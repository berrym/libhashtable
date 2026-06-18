project('libhashtable', 'c',
        default_options: 'c_std=c11',
        version: '0.9.1',
        license: 'MIT')

add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')
add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')

# Strict _XOPEN_SOURCE hides the BSD extensions (arc4random_buf, getentropy)
# that the entropy backend uses on macOS; _DARWIN_C_SOURCE restores them.
if host_machine.system() == 'darwin'
  add_project_arguments('-D_DARWIN_C_SOURCE', language: 'c')
endif

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')

if get_option('examples')
  subdir('examples')
endif
