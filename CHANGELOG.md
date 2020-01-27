# Changelog

## 3.0.1

- Use maintained `gyp` version
- Add check if layer extent (size!) is !=0 to avoid devisions by zero + default initialise `scale_` to 1.0
- Fix bunch of compiler warnings e.g `warning: explicitly defaulted copy constructor is implicitly deleted [-Wdefaulted-function-deleted]`
- Fix name resultion error (`../src/vector_tile_geometry_encoder_pbf.ipp:70:16: error: call to 'encode_geometry_pbf' is ambiguousreturn encode_geometry_pbf(geom, feature_, x_, y_);`)
- Fix issue with command lengths causing extra executions of a loop in tileinfo

## 3.0.0

- Set up layer resolution as layer extent (4096 by default) per width or height. Previously, if the layer had a datasource of type Vector the resolution would be set as 256 per layer width or height which caused some incorrect interactions with some Mapnik plugins that use this value to modify geometries. From now on, these operations will be done depending on the layer extent and not the default 256.
This change will only affect those that are using the Postgis plugin with simplification enabled (`simplify_distance : true`) or the Pgraster plugin using overviews (`use_overviews : true`) or raster prescaling (`prescale_rasters: true`). None of the options above are the default ones for those plugins.

## 2.2.1

- Fix bug where on linestring and point data the clipping box passed to boost geometry was malformed.

## 2.2.0

- Changed the underlying storage of the tile buffer to be a unique ptr controlled string so that it could be passed off if necessary outside the tile class.

## 2.1.1

- Fixed packaging to include all relevant commits from 2.1.0

## 2.1.0

- Fixed handling of value type conversion/storage (#280)
- Minimized geometry conversions to clean up code duplication (#282)

## 2.0.0

- Release for new mapnik series compatability (3.1.x or possibly 4.0.0).

## 1.6.1

- Solved problem surrounded odd reprojection issues when dealing with low zoom level tiles and projections covering small geographic areas.

## 1.6.0

- Bug fix for possible massive allocations in invalid vector tiles
- Fixed mercator bounding box code so that it does not require tile size, removed spherical mercator class
- Added the ability to use SSE code to improve the performance of simplification prior to vector tile creation.

## 1.5.0

- Added back ability to build against external mapnik (see docs for instructions)
- Added support for variables @rafatower: https://github.com/mapbox/mapnik-vector-tile/pull/248

## 1.4.0

- Fixed a bug associated with image height and width when reading from an image resulting in a size of zero causing exceptions.
- Updated to use mapnik 3.0.14, previous version of mapnik will not work properly with this newest version.
- Corrected issues with resolution associated with mapnik queries that was allowing the buffer size of the vector tile to affect the resolution.
- Removed some duplicate code
- Removed fuzzer from library

## 1.3.0

- Updated protozero to 1.5.1
- Changed build system to build against preset version of mapnik and dependencies in mason
- Changed to use wagyu rather then the angus clipper.
- Fixed bug associated with reprojections.

## 1.2.2

- Upgraded to protozero 1.4.2
- Added ability to dynamically get include paths by requiring module in node.js

## 1.2.1

- Updated clipper
- Upgrade to clang-3.8
- Works with latest variant (stricter type matching)

## 1.2.0

- Big overhaul to the interface around the vector tile decoder
- Slight performance increase in decoder
- Fixed a bug around throwing on incorrect winding order incorrectly when the exterior ring in a polygon was dropped by its extent but an interior ring was included.

## 1.1.2

- Fix performance regression when passing raster through vector tile

## 1.1.1

- Corrected for numerical precision issue when using decoder where it was incorrectly considering very small triangles as having zero area.

## 1.1.0

- Changed defaults for `merge_from_buffer`. Now the tile loading API does not auto-upgrade from v1->v2 or validate by default.
  The `upgrade` and `validate` behavior are now options to `merge_from_buffer` and `merge_from_compressed_buffer`

## 1.0.6

- Removed boost simplification and implemented custom douglas peucker for big speed boost.
- Updated the version of the clipper used.

## 1.0.5

- Several updates to the version of the clipper used.
- Removed the code and its requirements in `vector_tile_geometry_intersects.hpp` as it is no longer used.

## 1.0.4

- Updated the version of the clipper again, fixing more problems with intersections.
- Fixed bug in `vector_tile_geometry_feature.hpp` that was causing a segfault.

## 1.0.3

- Updated the version of clipper after a bug was found in clipper that would result in invalid polygons where interior rings were outside another exterior ring.
- Fixed a bug in `bench/vtile-encode.cpp`
- Fixed an issue with mapnik core no longer having `to_utf8` in the same directory in `vector_tile_layer.ipp`

## 1.0.2

- Added more errors and checks to `vector_tile_is_valid.hpp`

## 1.0.1

- Updated to protozero v1.3.0

## 1.0.0

Extensive redesign in mapnik-vector-tile to properly support 2.0 of the [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec/). A large number of changes have occured but a summary of these changes can be described as:

 - Removed `backend_pbf`
 - Changed `processor` interface
 - Removed requirement on libprotobuf `Tile` class when using the library.
 - Created new `tile` and `merc_tile` class
 - Added different processing logic for v1 and v2 decoding
 - Solved several small bugs around decoding and encoding
 - Added many more exceptions around the processing of invalid tiles.
 - Added `load_tile` and `composite` headers
 - Organized tests directory and added many more tests
 - Removed the concept of `is_solid`
 - Removed the concept of `path_multiplier`
 - Fixed bugs in `empty` concept.
 - `tile_size` is now directly related to the layer `extent`
 - Encoding no longer allows repeated points in lines and polygons
 - Corrected issues with winding order being reversed in some situations when decoding polygons
 - Changed the default configuration values for `processor`

## 0.14.3

- Fixed compile against latest Mapnik master (variant upgrade)

## 0.14.2

- Fixed premultiplication bug in raster encoding (#170)

## 0.14.1

- Had error in publish, republishing.

## 0.14.0

 - Added the ability for the processor to continue processing invalid rings rather than throwing them out. This is exposed via the option `process_all_rings`. (default is `false` meaning that all interior rings will be skipped if an invalid exterior ring is encountered)
 - Exposed the ability for different fill types to be used (default is `NonZero` meaning that winding order of input polygons is expected to be reversed for interior rings vs exterior rings)
 - Added the ability for multipolygons to be unioned or not, exposed as option `multipoly_polyon_union` (default is `true` meaning that overlapping polygons in a multipolygon will be unioned)
 - Added new test suite for geometries to increase code coverage

## 0.13.0

 - Updated the geometry decoder so that it now supports a variety of geometry formats with the ability to return mapnik
   geometries in value types other then doubles.

## 0.12.1

 - Removed repeated points of linestrings prior to them being encoded.

## 0.12.0

 - Reversed the winding order of the geometries that comes out of the angus clipper so they are not reversed again prior to encoding
 - Fixed an issue with nonZero fill not being applied on multipolygons
 - Removed unrequired unioning clipping operations as union of different paths occurs during the intersection operation.

## 0.11.0

 - Changed processor so that it now can optionally turn on and off strict enforcing of simple geometries from the clipper
 - Updated angus clipper library used in Makefile to 6.4.0 (https://github.com/mapnik/clipper/tree/r496-mapnik)

## 0.10.0

 - Changed the way painted is determined. Painted could not be marked as true but a vector tile would still be empty.

## 0.9.3

 - Improvements to zlib compression API

## 0.9.2

 - Fixed multipoint encoding (#144)
 - Optimized decoding by filtering geometry parts not within bbox (#146)
 - Optimized decoding by calling `vector.reserve` before `vector.emplace_back` (#119)

## 0.9.1

 - Added `is_solid_extent` implementation based on protozero decoder

## 0.9.0

 - Upgraded to protozero v1.0.0
 - Fixed attribute handling bug in tile_datasource_pbf

## 0.8.5

 - Remove geometries from clipping that never intersect with the bounding box of the tile (#135)
 - Fix indexing error in tile_datasource_pbf (#132)

## 0.8.4

 - Started to skip coordinates that are out of range (#121)
 - Fix clipping box used when reprojecting data on the fly (#128)
 - Fixed decoding of degenerate polygons - we need to gracefully support these as they
   are commonly in the wild based on that AGG clipper used in v0.7.1 and earlier (#123)

## 0.8.3

 - Started to skip coordinates that cannot be reprojected (#117)
 - Minor optimization in attribute encoding by using `emplace` instead of `insert`
 - Now depends on `pbf_writer.hpp` for zigzag implementation (no change in behavior)
 - Minor code cleanup to avoid unnecessary compiler warnings

## 0.8.2

 - Optimized coordinate transform that skips proj4 failures (#116)

## 0.8.1

 - Added `tile_datasource_pbf` - It should be used in places where you need to plug
   in a `mapnik::datasource` to read from a binary encoded .pbf buffer. (@danpat #114)
 - Updated bundled clipper to https://github.com/mapnik/clipper/commit/bfad32ec4b41783497d076c2ec44c7cbf4ebe56b
 - Clipper is now patched to avoid abort on out of range coordinates (#111)
 - Fixed handling of geometry collections (#106)
 - Added mapnik vector tile strategy for transform
 - Updated test cases

## 0.8.0

 - Now using `boost::geometry` to clip lines and `ClipperLib` to clip polygons
 - Now splitting geometry collections into multiple features
 - Updated to new Mapnik 3.x geometry storage
 - Added support for simplifying geometries using `boost::geometry::simplify`
 - Added `area_threshold` option to throw out small polygons

## 0.7.1

 - Minor build fixes

## 0.7.0

 - First release series to exclusively focus on upcoming Mapnik 3.x release.

## 0.6.2

 - The 0.6.x series will no longer support Mapnik 3.x going forward and will
   instead only maintain Mapnik 2.x support
 - Minor fixes to compile against latest Mapnik 3.x (works with ec2d644f6b698f)

## 0.6.1

 - `tile_datasource` now has optional 6th arg to trigger exploding multipart geometries when decoding

## 0.6.0

 - Adapted to vector tile v1.0.1 spec
 - Fixed compile with g++ / clashing namespace with protobuf

## 0.5.6

 - Fix build against latest Mapnik 3.x

## 0.5.5

 - Optimize raster rendering by clipping rasters to unbuffered tile extent when overzooming

## 0.5.4

 - Fixed bug in line intersection test

## 0.5.3

 - Fixed setting of `painted` property when a raster is successfully added
 - Added support for testing line intersections in is_solid check
 - Updated to work with latest Mapnik 3.x
 - Improved test coverage

## 0.5.2

 - Build fixes to work against latest Mapnik 3.x

## 0.5.1

 - Minor build fixes

## 0.5.0

 - Experimental support for encoding images in vector tile features.
 - Fixed potential hang if trying to render a feature without geometries

## 0.4.2

 - Additional optimizations and fixes to geometry encoding (#38 - avoid dropping vertex that forms horizontal or vertical right angle)

## 0.4.1

 - Added initial support for Mapnik 3.x

## 0.4.0

 - Refactored geometry encoder with fixes to drop duplicated/no-op verticies and/or
  close commands
 - `npm install` no longer runs `protoc` - the responsibility for this is now up to `node-mapnik`
 - Optimized is_solid check

## 0.3.7

 - Add back protoc running to avoid unintended node-mapnik breakages with older versions.

## 0.3.6

 - Avoided 'Unknown command type (is_solid_extent): 0'

## 0.3.5

 - `npm install` no longer runs `protoc` - the responsibility for this is now up to `node-mapnik`
 - Improved tile encoding: empty layers are no longer added
 - All move_to commands and the last vertex in lines is no longer thrown out even with high `tolerance`
 - Rolled back the change from v0.3.4 - multipart geometries are now again not decoded correctly, but this
   needs to stay this way for performance reasons at the cost of correct marker/labeling placement on each
   geometry part - long term solutions tracked at mapnik/mapnik#2151. Re-enabled to v0.3.4 behavior by setting
   `-DEXPLODE_PARTS` in `CXXFLAGS`.

## 0.3.4

 - Fixed tile_datasource geometry decoding such that it polygons are closed (for hit_test results)
 - Fixed tile_datasource geometry decoding such that it respects multipart geometries (#19)

## 0.3.3

 - Added support in tileinfo demo program for reading zlib compressed tiles
 - Removed dependence on clipper.hp unless `-DCONV_CLIPPER` is defined in `CXXFLAGS`
 - Upgraded bundled cache.hpp test framework to `1.0 build 8` (kapouer)

## 0.3.2

 - Fixed `mapnik::vector::tile_datasource` to respect the feature id if known

## 0.3.1

 - Added support for reporting known attribute names for a given vector tile layer via `mapnik::vector::tile_datasource` `descriptors`.

## 0.3.0

 - API change: mapnik::vector::processor now requires a mapnik::request object as the third argument. The reason for this is to make it more viable for calling programs to avoid needing to mutate the map before passing to the processor. Now the `width`, `height`, `buffer_size`, and `extent` will be taken from the mapnik::request object inside processor.apply(). For now the `srs` and `layers` will still be taken off the map.

## 0.2.5

 - Fixed incorrect copy of protobuf writing backend when passed to the processor

## 0.2.4

 - Marked tile_datasource implemented to avoid duplicate symbol errors if used from multiple compilation units

## 0.2.3

 - Fixed casting between doubles and ints (solves test failures on 32 bit linux)

## 0.2.2

 - Fixed accuracy of bbox filtering of features

## 0.2.1

 - Removed tile_datasource validation that requested attributes exist in feature since current vector tiles do not guarantee that `tile_layer::keys` is populated with all possible attributes and rather only include those encounted by features processed.

## 0.2.0

 - Now filtering features based on mapnik::query bbox and filtering attributes based on mapnik::query names. (#6)

## 0.1.0

 - API change: Optimized spherical mercator math and reworked interface. `mapnik::vector::spherical_mercator` now is not templated on max zoom level and works with any zoom. `mapnik::vector::spherical_mercator::xyz` now expects references to doubles instead of a `mapnik::box2d`.

## 0.0.6

 - Removed stale and unused code

## 0.0.5

 - Packaging fix

## 0.0.4

 - Compile fix to use consistent headers

## 0.0.3

 - packaging fix

## 0.0.2

 - test / readme improvements

## 0.0.1

 - Initial release
