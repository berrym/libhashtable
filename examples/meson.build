example_names = ['wordcount', 'entity_index', 'record_store', 'request_router']

foreach name : example_names
  executable(name,
             name + '.c',
             include_directories: inc,
             link_with: libhashtable)
endforeach
