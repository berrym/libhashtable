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

test('libhashtable', test_ht_strstr_exe)
test('libhashtable', test_ht_strint_exe)
test('libhashtable', test_ht_strfloat_exe)
test('libhashtable', test_ht_strdouble_exe)
test('libhashtable', test_ht_fnv1a_collision_exe)
