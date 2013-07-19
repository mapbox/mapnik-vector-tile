# Changlog

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