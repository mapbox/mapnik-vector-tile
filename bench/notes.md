With reserve

```
$ ./build/Release/multi-point-decode bench/multi_line_13_1310_3166.vector.pbf 13 1310 3166
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4026.30ms (cpu 4020.96ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.pbf
```

without reserve code

```
$ ./build/Release/multi-point-decode bench/multi_line_13_1310_3166.vector.pbf 13 1310 3166
z:13 x:1310 y:3166 iterations:100
message: zlib compressed
4296.88ms (cpu 4289.05ms)   | decode as datasource_pbf: bench/multi_line_13_1310_3166.vector.pbf
```


