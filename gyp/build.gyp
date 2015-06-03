{
  "includes": [
      "common.gypi"
  ],
  'variables': {
    'MAPNIK_PLUGINDIR%': ''
  },
  "targets": [
    {
      'target_name': 'make_vector_tile',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'run_protoc',
          'inputs': [
            '../proto/vector_tile.proto'
          ],
          'outputs': [
            "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc",
            "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.h"
          ],
          'action': ['protoc','-I../proto/','--cpp_out=<(SHARED_INTERMEDIATE_DIR)/','../proto/vector_tile.proto']
        }
      ]
    },

    {
      "target_name": "vector_tile",
      'dependencies': [ 'make_vector_tile' ],
      'hard_dependency': 1,
      "type": "static_library",
      "sources": [
        "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc"
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/'
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(mapnik-config --cflags)' # assume protobuf headers are here
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '-D_THREAD_SAFE',
           '<!@(mapnik-config --cflags)' # assume protobuf headers are here
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/'
        ],
        'libraries':[
          '-lprotobuf-lite'
        ],
        'cflags_cc' : [
            '-D_THREAD_SAFE'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
             '-D_THREAD_SAFE',
          ],
        },
      }
    },
    {
      "target_name": "mapnik_vector_tile_impl",
      'dependencies': [ 'vector_tile' ],
      'hard_dependency': 1,
      "type": "static_library",
      "sources": [
        "<!@(find ../src/ -name '*.cpp')"
      ],
      "include_dirs": [
        "../deps/pbf"
      ],
      'defines' : [
        'MAPNIK_VECTOR_TILE_LIBRARY=1'
      ],
      'cflags_cc' : [
          '<!@(mapnik-config --cflags)'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(mapnik-config --cflags)'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/'
        ],
        'defines' : [
          'MAPNIK_VECTOR_TILE_LIBRARY=1'
        ],
        'cflags_cc' : [
            '<!@(mapnik-config --cflags)'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
             '<!@(mapnik-config --cflags)'
          ],
        },
        'libraries':[
          '<!@(mapnik-config --libs)',
          '<!@(mapnik-config --ldflags)',
          '-lmapnik-wkt',
          '-lmapnik-json',
          '<!@(mapnik-config --dep-libs)',
          '-lprotobuf-lite',
          '-lz'
        ],
      }
    },
    {
      "target_name": "tests",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ], 
      "sources": [
        "<!@(find ../test/ -name '*.cpp')"
      ],
      "include_dirs": [
        "../src",
        "../deps/pbf"
      ]
    },
    {
      "target_name": "tileinfo",
      'dependencies': [ 'vector_tile' ],
      "type": "executable",
      "sources": [
        "../examples/c++/tileinfo.cpp"
      ],
      "include_dirs": [
        "../src"
      ],
      'libraries':[
        '-L<!@(mapnik-config --prefix)/lib',
        '<!@(mapnik-config --ldflags)',
        '-lz'
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(mapnik-config --cflags)' # assume protobuf headers are here
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '-D_THREAD_SAFE',
           '<!@(mapnik-config --cflags)' # assume protobuf headers are here
        ],
      }
    }    

  ]
}