libhashtable_sources = ['ht.c',
                        'ht_strstr.c',
                        'ht_strint.c',
                        'ht_strfloat.c',
                        'ht_strdouble.c',
                        'ht_fnv1a.c',
                        'ht_random.c']

libhashtable_deps = []
if host_machine.system() == 'windows'
  libhashtable_deps += meson.get_compiler('c').find_library('bcrypt')
endif

libhashtable = library('hashtable',
                       libhashtable_sources,
                       include_directories : inc,
                       dependencies : libhashtable_deps,
                       install : true)
