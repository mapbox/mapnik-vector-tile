# Contributing

General guidelines for contributing to mapnik-vector-tile

## Releasing

To release a new mapnik-vector-tile version:

 - Make sure that all tests as passing (including travis tests).
 - Update the CHANGELOG.md
 - Make a "bump commit" by updating the version in `package.json` and adding a commit like `-m "bump to v0.8.5"`
 - Create a github tag like `git tag -a v0.8.5 -m "v0.8.5" && git push --tags`
 - Ensure travis tests are passing
 - Ensure you have a clean checkout (no extra files in your check that are not known by git). You need to be careful, for instance, to avoid a large accidental file being packaged by npm. You can get a view of what npm will publish by running `make testpack`
 - Then publish the module to npm repositories by running `npm publish`
