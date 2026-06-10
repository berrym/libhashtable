libhashtable_sources = ['ht.c',
                        'ht_strstr.c',
                        'ht_strint.c',
                        'ht_strfloat.c',
                        'ht_strdouble.c',
                        'ht_fnv1a.c',
                        'ht_random.c',
                        'ht_int.c',
                        'ht_bulk.c',
                        'ht_siphash.c',
                        'ht_strblob.c',
                        'ht_u64ptr.c',
                        'ht_u64blob.c',
                        'ht_strptr.c',
                        'ht_strset.c']

libhashtable_deps = []
if host_machine.system() == 'windows'
  libhashtable_deps += meson.get_compiler('c').find_library('bcrypt')
endif

libhashtable = library('hashtable',
                       libhashtable_sources,
                       include_directories : inc,
                       dependencies : libhashtable_deps,
                       install : true)

# Consumable as a subproject: dependency('libhashtable', fallback: ...)
# resolves to this, and override_dependency lets a plain dependency lookup
# find it when this project is wrapped. Static vs shared follows the
# consumer's default_library setting.
libhashtable_dep = declare_dependency(
  link_with : libhashtable,
  include_directories : inc)

meson.override_dependency('libhashtable', libhashtable_dep)
