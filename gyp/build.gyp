{
  "includes": [
      "common.gypi"
  ],
  'variables': {
    'MAPNIK_PLUGINDIR%': '',
    'common_defines' : [
        'MAPNIK_VECTOR_TILE_LIBRARY=1',
        'CLIPPER_INTPOINT_IMPL=mapnik::geometry::point<cInt>',
        'CLIPPER_PATH_IMPL=mapnik::geometry::line_string<cInt>',
        'CLIPPER_PATHS_IMPL=mapnik::geometry::multi_line_string<cInt>',
        'CLIPPER_IMPL_INCLUDE=<mapnik/geometry.hpp>'
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
        '../deps/protozero/include'
      ],
      'cflags_cc' : [
          '-D_THREAD_SAFE',
          '<!@(mapnik-config --cflags)', # assume protobuf headers are here
          '-Wno-sign-compare',
          '-Wno-sign-conversion'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '-D_THREAD_SAFE',
           '<!@(mapnik-config --cflags)', # assume protobuf headers are here
           '-Wno-sign-compare',
           '-Wno-sign-conversion'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/',
          '../deps/protozero/include'
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
        "<!@(find ../src/ -name '*.cpp')",
        "../deps/clipper/cpp/clipper.cpp"
      ],
      'defines' : [
        "<@(common_defines)"
      ],
      'cflags_cc' : [
          '<!@(mapnik-config --cflags)'
      ],
      'include_dirs': [
        '../deps/protozero/include',
        '../deps/clipper/cpp'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(mapnik-config --cflags)'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/',
          '../deps/protozero/include',
          '../deps/clipper/cpp'
        ],
        'defines' : [
          "<@(common_defines)"
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
        "<@(common_defines)",
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ],
      "sources": [
        "<!@(find ../test/ -name '*.cpp')"
      ],
      "include_dirs": [
        "../src",
        '../deps/protozero/include',
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
        '../deps/protozero/include'
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
        '../deps/protozero/include'
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
        '../deps/protozero/include'
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
        '../deps/protozero/include'
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
        '../deps/protozero/include'
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
