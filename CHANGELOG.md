# Changelog

## 0.8.1

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
