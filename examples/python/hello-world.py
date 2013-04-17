#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import json
# put `./lib` dir on path
sys.path.append("./python")

import renderer

if __name__ == "__main__" :
    # create a single tile at 0/0/0.png like tile.osm.org/0/0/0.png
    zoom = 0
    x = 0
    y = 0
    # request object holds a Tile XYZ and internally holds mercator extent
    req = renderer.Request(x,y,zoom)
    # create a vector tile, given a tile request
    vtile = renderer.VectorTile(req)
    # for a given point representing a spot in NYC
    lat = 40.70512
    lng = -74.01226
    # and some attributes
    attr = {"hello":"world"}
    # convert to mercator coords
    x,y = renderer.lonlat2merc(lng,lat)
    # add this point and attributes to the tile
    vtile.add_point(x,y,attr)
    # print the protobuf as geojson just for debugging
    # NOTE: coordinate rounding is by design and 
    print 'GeoJSON representation of tile (purely for debugging):'
    print vtile.to_geojson()
    print '-'*60
    # print the protobuf message as a string
    print 'Protobuf string representation of tile (purely for debugging):'
    print vtile
    print '-'*60
    # print the protobuf message, zlib encoded
    print 'Serialized, deflated tile message (storage and wire format):'
    print vtile.to_message().encode('zlib')
