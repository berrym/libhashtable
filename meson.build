project('libhashtable', 'c',
        default_options: 'c_std=c11',
        version: '0.7.0',
        license: 'MIT')

# Hash width override. 'auto' lets the header pick from UINTPTR_MAX so the
# typed wrappers and FNV-1a constants agree with the host word size. Force
# '32' or '64' for hash-stability or cross-compile test vectors.
hash_width = get_option('hash_width')
if hash_width != 'auto'
  add_project_arguments('-DHT_HASH_WIDTH=' + hash_width, language: 'c')
endif

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-DXOPEN=600', language: 'c')

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')
