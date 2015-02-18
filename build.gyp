{
  "includes": [
      "common.gypi"
  ],
  'variables': {
    'MAPNIK_PLUGINDIR%': ''
  },
  "targets": [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'run_protoc',
          'inputs': [
            './proto/vector_tile.proto'
          ],
          'outputs': [
            "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc",
            "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.h"
          ],
          'action': ['protoc','-Iproto/','--cpp_out=<(SHARED_INTERMEDIATE_DIR)/','./proto/vector_tile.proto']
        }
      ]
    },
    {
      "target_name": "vector_tile",
      'dependencies': [ 'action_before_build' ],
      "type": "static_library",
      "sources": [
        "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc",
        "src/vector_tile_util.cpp",
        "src/vector_tile_projection.cpp",
        "src/vector_tile_geometry_encoder.cpp",
        "src/vector_tile_datasource.cpp",
        "src/vector_tile_compression.cpp",
        "src/vector_tile_backend_pbf.cpp",
        "src/vector_tile_processor.cpp",
      ],
      'defines' : [
        'MAPNIK_VECTOR_TILE_LIBRARY=1'
      ],
      'libraries':[
          '<!@(pkg-config protobuf --libs-only-L)',
          '-lprotobuf-lite',
      ],
      'cflags_cc' : [
          '<!@(pkg-config protobuf --cflags)'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(pkg-config protobuf --cflags)',
        ],
      },
      'cflags_cc' : [
          '<!@(mapnik-config --cflags)'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(mapnik-config --cflags)'
        ],
      },
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/'
      ],

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
        'libraries':[
          '<!@(mapnik-config --libs)',
          '<!@(mapnik-config --ldflags)',
          '-lprotobuf-lite',
          '-lz'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
             '<!@(mapnik-config --cflags)'
          ],
        },
      }
    },
    {
      "target_name": "tests",
      'dependencies': [ 'vector_tile' ],
      "type": "executable",
      "defines": [
        "MAPNIK_PLUGINDIR=<(MAPNIK_PLUGINDIR)"
      ], 
      "sources": [
        "./test/test_main.cpp",
        "./test/test_utils.cpp",
        "./test/geometry_encoding.cpp",
        "./test/vector_tile.cpp",
        "./test/raster_tile.cpp",
      ],
      "include_dirs": [
        "./src"
      ]
    }    
  ]
}