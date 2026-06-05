test_ht_strstr_exe = executable('test_ht_strstr',
			                    'ht_strstr_test.c',
			                    include_directories : inc,
			                    link_with : libhashtable)

test_ht_strint_exe = executable('test_ht_strint',
			                    'ht_strint_test.c',
			                    include_directories : inc,
			                    link_with : libhashtable)

test_ht_strfloat_exe = executable('test_ht_strfloat',
			                      'ht_strfloat_test.c',
			                      include_directories : inc,
			                      link_with : libhashtable)

test_ht_strdouble_exe = executable('test_ht_strdouble',
			                       'ht_strdouble_test.c',
			                       include_directories : inc,
			                       link_with : libhashtable)

test_ht_fnv1a_collision_exe = \
  executable('test_ht_fnv1a_collision',
             'ht_fnv1a_collision_test.c',
             include_directories: inc,
             link_with: libhashtable)

test_ht_random_exe = executable('test_ht_random',
			                    'ht_random_test.c',
			                    include_directories : inc,
			                    link_with : libhashtable)

test_ht_int_exe = executable('test_ht_int',
			                 'ht_int_test.c',
			                 include_directories : inc,
			                 link_with : libhashtable)

test_ht_bulk_exe = executable('test_ht_bulk',
			                  'ht_bulk_test.c',
			                  include_directories : inc,
			                  link_with : libhashtable)

test_ht_siphash_exe = executable('test_ht_siphash',
			                     'ht_siphash_test.c',
			                     include_directories : inc,
			                     link_with : libhashtable)

test('libhashtable', test_ht_strstr_exe)
test('libhashtable', test_ht_strint_exe)
test('libhashtable', test_ht_strfloat_exe)
test('libhashtable', test_ht_strdouble_exe)
test('libhashtable', test_ht_fnv1a_collision_exe)
test('libhashtable', test_ht_random_exe)
test('libhashtable', test_ht_int_exe)
test('libhashtable', test_ht_bulk_exe)
test('libhashtable', test_ht_siphash_exe)
