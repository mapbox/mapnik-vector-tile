# Contributing

General guidelines for contributing to mapnik-vector-tile

## Releasing

To release a new node-mapnik version:

 - Make sure that all tests as passing (including travis and appveyor tests).
 - Update the CHANGELOG.md and commit new version.
 - Create a github tag like `git tag -a v0.8.5 -m "v0.8.5"`
 - Ensure you have a clean checkout (no extra files in your check that are not known by git). You need to be careful, for instance, to avoid a large accidental file being packaged by npm. You can get a view of what npm will publish by running `make testpack`
 - Then publish the module to npm repositories by running `npm publish`
