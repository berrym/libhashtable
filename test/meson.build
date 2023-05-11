test_ht_strstr_exe = executable('test_ht_strstr',
			'ht_strstr_test.c',
			include_directories : inc,
			link_with : lib_hashtable)

test_ht_strint_exe = executable('test_ht_strint',
			'ht_strint_test.c',
			include_directories : inc,
			link_with : lib_hashtable)

test('lib_hashtable', test_ht_strstr_exe)
test('lib_hashtable', test_ht_strint_exe)
