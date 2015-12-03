With reserve

```
$ ./build/Release/vtile-decode bench/multi_line_13_1310_3166.vector.mvt 13 1310 3166
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4026.30ms (cpu 4020.96ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.mvt
```

without reserve code

```
$ ./build/Release/vtile-decode bench/multi_line_13_1310_3166.vector.mvt 13 1310 3166
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4296.88ms (cpu 4289.05ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.mvt
```


---------


baseline (using mapnik::geometry::envelope + filter.pass)

$ .//build/Release/vtile-decode bench/enf.t5yd5cdi_14_13089_8506.vector.mvt 14 13089 8506 200
z:14 x:13089 y:8506 iterations:200
2822.66ms (cpu 2821.10ms)   | decode as datasource_pbf: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
2070.90ms (cpu 2070.44ms)   | decode as datasource: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
processed 6800 features

$ ./build/Release/vtile-decode bench/multi_line_13_1310_3166.vector.mvt 13 1310 3166 100
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4289.26ms (cpu 4275.29ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.mvt



commenting filter.pass/geometry::envelope in datasource_pbf

$ .//build/Release/vtile-decode bench/enf.t5yd5cdi_14_13089_8506.vector.mvt 14 13089 8506 200
z:14 x:13089 y:8506 iterations:200
2305.45ms (cpu 2301.07ms)   | decode as datasource_pbf: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
2142.56ms (cpu 2140.40ms)   | decode as datasource: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
processed 6800 features


with bbox filter:

$ .//build/Release/vtile-decode bench/enf.t5yd5cdi_14_13089_8506.vector.mvt 14 13089 8506 200
z:14 x:13089 y:8506 iterations:200
2497.01ms (cpu 2493.40ms)   | decode as datasource_pbf: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
1753.55ms (cpu 1751.10ms)   | decode as datasource: bench/enf.t5yd5cdi_14_13089_8506.vector.mvt
processed 6600 features

$ ./build/Release/vtile-decode bench/multi_line_13_1310_3166.vector.mvt 13 1310 3166 100
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4090.67ms (cpu 4083.65ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.mvt
