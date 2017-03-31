{
  "includes": [
      "common.gypi"
  ],
  'variables': {
    'MAPNIK_PLUGINDIR%': '',
    'common_defines' : [
        'MAPNIK_VECTOR_TILE_LIBRARY=1',
    ]
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
        '<(SHARED_INTERMEDIATE_DIR)/',
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(../mason_packages/.link/bin/mapnik-config --cflags)', # assume protobuf headers are here
          '-Wno-sign-compare',
          '-Wno-sign-conversion'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '-D_THREAD_SAFE',
           '<!@(../mason_packages/.link/bin/mapnik-config --cflags)', # assume protobuf headers are here
           '-Wno-sign-compare',
           '-Wno-sign-conversion'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/',
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
      'defines' : [
        "<@(common_defines)"
      ],
      'cflags_cc' : [
          '<!@(../mason_packages/.link/bin/mapnik-config --cflags)'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(../mason_packages/.link/bin/mapnik-config --cflags)'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/',
        ],
        'defines' : [
          "<@(common_defines)"
        ],
        'cflags_cc' : [
            '<!@(../mason_packages/.link/bin/mapnik-config --cflags)'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
             '<!@(../mason_packages/.link/bin/mapnik-config --cflags)'
          ],
        },
        'libraries':[
          '<!@(../mason_packages/.link/bin/mapnik-config --libs)',
          '<!@(../mason_packages/.link/bin/mapnik-config --ldflags)',
          '-lmapnik-wkt',
          '-lmapnik-json',
          '<!@(../mason_packages/.link/bin/mapnik-config --dep-libs)',
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
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "<!@(find ../test/ -name '*.cpp')"
      ],
      "include_dirs": [
        "../src",
        '../test',
        '../test/utils'
      ]
    },
    {
      "target_name": "vtile-transform",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "../bench/vtile-transform.cpp"
      ],
      "include_dirs": [
        "../src",
      ]
    },
    {
      "target_name": "vtile-decode",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "../bench/vtile-decode.cpp"
      ],
      "include_dirs": [
        "../src",
      ]
    },
    {
      "target_name": "vtile-encode",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "../bench/vtile-encode.cpp"
      ],
      "include_dirs": [
        "../src",
      ]
    },
    {
      "target_name": "vtile-edit",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "../bin/vtile-edit.cpp"
      ],
      "include_dirs": [
        "../src",
      ]
    },
    {
      "target_name": "vtile-fuzz",
      'dependencies': [ 'mapnik_vector_tile_impl' ],
      "type": "executable",
      "defines": [
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "../bin/vtile-fuzz.cpp"
      ],
      "include_dirs": [
        "../src",
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
        '-L<!@(../mason_packages/.link/bin/mapnik-config --prefix)/lib',
        '<!@(../mason_packages/.link/bin/mapnik-config --ldflags)',
        '-lz'
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(../mason_packages/.link/bin/mapnik-config --cflags)' # assume protobuf headers are here
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '-D_THREAD_SAFE',
           '<!@(../mason_packages/.link/bin/mapnik-config --cflags)' # assume protobuf headers are here
        ],
      }
    }    

  ]
}
