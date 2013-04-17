#!/usr/bin/env python

import sys
import gzip
import json
import os
import math
import vector_tile_pb2

class Box2d(object):
    """Box2d object to represent floating point bounds as
    
        minx,miny,maxx,maxy
     
     Which we can also think of as:

        left,bottom,right,top
        west,south,east,north
    """
    def __init__(self,minx,miny,maxx,maxy):
        self.minx = float(minx)
        self.miny = float(miny)
        self.maxx = float(maxx)
        self.maxy = float(maxy)

    def width(self):
        return self.maxx - self.minx

    def height(self):
        return self.maxy - self.miny
    
    def bounds(self):
        return [self.minx,self.miny,self.maxx,self.maxy]
    
    def intersects(self, x, y):
        return not (x > self.maxx or x < self.minx or y > self.maxy or y < self.miny)

    def __repr__(self):
        return 'Box2d(%s,%s,%s,%s)' % (self.minx,self.miny,self.maxx,self.maxy)

# Max extent edge for spherical mercator
MAX_EXTENT = 20037508.342789244
DEG_TO_RAD = math.pi/180
RAD_TO_DEG = 180/math.pi

def lonlat2merc(lon,lat):
    "Convert coordinate pair from epsg:4326 to epsg:3857"
    x = lon * MAX_EXTENT / 180
    y = math.log(math.tan((90 + lat) * math.pi / 360)) / DEG_TO_RAD
    y = y * MAX_EXTENT / 180
    return [x,y]

def merc2lonlat(x,y):
    "Convert coordinate pair from epsg:3857 to epsg:4326"
    x = (x / MAX_EXTENT) * 180;
    y = (y / MAX_EXTENT) * 180;
    y = RAD_TO_DEG * (2 * math.atan(math.exp(y * DEG_TO_RAD)) - math.pi/2);
    return x,y

def minmax(a,b,c):
    a = max(a,b)
    a = min(a,c)
    return a

class SphericalMercator(object):
    """
    Core definition of Spherical Mercator Projection.
    
    Adapted from:  
      http://svn.openstreetmap.org/applications/rendering/mapnik/generate_tiles.py
    """
    def __init__(self, levels=22, size=256):
        self.Bc = []
        self.Cc = []
        self.zc = []
        self.Ac = []
        self.levels = levels
        self.size = size
        for d in range(0,levels+1):
            e = size/2.0;
            self.Bc.append(size/360.0)
            self.Cc.append(size/(2.0 * math.pi))
            self.zc.append((e,e))
            self.Ac.append(size)
            size *= 2.0

    def ll_to_px(self, px, zoom):
        """ Convert LatLong (EPSG:4326) to pixel postion """
        d = self.zc[zoom]
        e = round(d[0] + px[0] * self.Bc[zoom])
        f = minmax(math.sin(DEG_TO_RAD * px[1]),-0.9999,0.9999)
        g = round(d[1] + 0.5 * math.log((1+f)/(1-f))*-self.Cc[zoom])
        return (e,g)
    
    def px_to_ll(self, px, zoom):
        """ Convert pixel postion to LatLong (EPSG:4326) """
        e = self.zc[zoom]
        f = (px[0] - e[0])/self.Bc[zoom]
        g = (px[1] - e[1])/-self.Cc[zoom]
        h = RAD_TO_DEG * ( 2 * math.atan(math.exp(g)) - 0.5 * math.pi)
        return (f,h)
    
    def bbox(self, x, y, zoom):
        """ Convert XYZ to extent in mercator """
        ll = (x * self.size,(y + 1) * self.size)
        ur = ((x + 1) * self.size, y * self.size)
        minx,miny = self.px_to_ll(ll,zoom)
        maxx,maxy = self.px_to_ll(ur,zoom)
        minx,miny = lonlat2merc(minx,miny)
        maxx,maxy = lonlat2merc(maxx,maxy)
        return [minx,miny,maxx,maxy]

    def xyz(self, bbox, zoom):
        """ Convert extent in mercator to XYZ extents"""
        minx,miny,maxx,maxy = bbox
        ll = merc2lonlat(minx,miny)
        ur = merc2lonlat(maxx,maxy)
        px_ll = self.ll_to_px(ll,zoom)
        px_ur = self.ll_to_px(ur,zoom)
        return [int(math.floor(px_ll[0]/self.size)),
                int(math.floor(px_ur[1]/self.size)),
                int(math.floor((px_ur[0]-1)/self.size)),
                int(math.floor((px_ll[1]-1)/self.size))
               ]

class Request(object):
    """
    Request encapulates a single tile request in the common OSM, aka XYZ scheme.
    
    Interally we convert the x,y,zoom to a mercator bounding box assuming a 256 pixel tile
    """
    def __init__(self, x, y, zoom):
        assert isinstance(zoom,int)
        assert zoom <= 22
        assert isinstance(x,int)
        assert isinstance(y,int)
        self.x = x
        self.y = y
        self.zoom = zoom
        self.size = 256
        self.mercator = SphericalMercator(levels=22,size=self.size)
        self.extent = Box2d(*self.mercator.bbox(x,y,zoom))
    
    def get_extent(self):
        return self.extent

    def get_width(self):
        return self.extent.width()

    def get_height(self):
        return self.extent.height()
    
    def bounds(self):
        return self.extent.bounds()

class CoordTransform(object):
    """
    CoordTransform provides methods for converting coordinate pairs
    to and from a geographical coordinate system (usually mercator)
    to screen or pixel coordinates for a given tile request.
    """
    def __init__(self,request):
        assert isinstance(request,Request)
        self.extent = request.get_extent()
        self.sx = (float(request.size) / request.get_width())
        self.sy = (float(request.size) / request.get_height())

    def forward(self,x,y):
        """Geo coordinates to Screen coordinates"""
        x0 = (x - self.extent.minx) * self.sx
        y0 = (self.extent.maxy - y) * self.sy
        return x0,y0

    def backward(self,x,y):
        """Screen coordinates to Geo coordinates"""
        x0 = self.extent.minx + x / self.sx
        y0 = self.extent.maxy - y / self.sy
        return x0,y0

class VectorTile(object):
    """
    VectorTile is object that makes it easy to turn a sequence of
    point features into a vector tile using optimized encoding for
    transport over the wire and later rendering by MapBox tools.

    """
    def __init__(self, req, path_multiplier=16):
        assert isinstance(req,Request)
        self.request = req
        self.extent = self.request.extent
        self.ctrans = CoordTransform(req)
        self.path_multiplier = path_multiplier
        self.tile = vector_tile_pb2.tile()
        self.layer = self.tile.layers.add()
        self.layer.name = "points"
        self.layer.version = 2
        self.layer.extent = req.size * self.path_multiplier # == 4096
        self.feature_count = 0
        self.keys = []
        self.values = []
        self.pixels = []
    
    def __str__(self):
        return self.tile.__str__()

    def to_message(self):
        return self.tile.SerializeToString()

    def _decode_coords(self, dx, dy):
        x = ((dx >> 1) ^ (-(dx & 1)))
        y = ((dy >> 1) ^ (-(dy & 1)))
        x,y = float(x)/self.path_multiplier,float(y)/self.path_multiplier
        x,y = self.ctrans.backward(x,y);
        return x,y

    def _encode_coords(self, x, y, rint=False):
        dx,dy = self.ctrans.forward(x,y)
        if rint:
            dx = int(round(dx * self.path_multiplier))
            dy = int(round(dy * self.path_multiplier))
        else:
            dx = int(math.floor(dx * self.path_multiplier))
            dy = int(math.floor(dy * self.path_multiplier))
        dxi = (dx << 1) ^ (dx >> 31)
        dyi = (dy << 1) ^ (dy >> 31)
        #assert dxi >= 0 and dxi <= self.layer.extent
        #assert dyi >= 0 and dyi <= self.layer.extent
        return dxi,dyi

    def add_point(self, x, y, properties,skip_coincident=True,rint=False):
        if self.extent.intersects(x,y):
            dx,dy = self._encode_coords(x,y,rint=rint)
            # TODO - use numpy matrix to "paint" points so we
            # can drop all coincident ones except last
            key = "%d-%d" % (dx,dy)
            if key not in self.pixels:
                f = self.layer.features.add()
                self.feature_count += 1
                f.id = self.feature_count
                f.type = self.tile.Point
                self._handle_attr(self.layer,f,properties)
                f.geometry.append((1 << 3) | (1 & ((1 << 3) - 1)))
                f.geometry.append(dx)
                f.geometry.append(dy)
                self.pixels.append(key)
            return True
        else:
            raise RuntimeError("point does not intersect with tile bounds")
        return False

    def to_geojson(self,lonlat=False):
        jobj = {}
        jobj['type'] = "FeatureCollection"
        features = []
        for feat in self.layer.features:
            fobj = {}
            fobj['type'] = "Feature"
            properties = {}
            for i in xrange(0,len(feat.tags),2):
                key_id = feat.tags[i]
                value_id = feat.tags[i+1]
                name = str(self.layer.keys[key_id])
                val = self.layer.values[value_id]
                if val.HasField('bool_value'):
                    properties[name] = val.bool_value
                elif val.HasField('string_value'):
                    properties[name] = val.string_value
                elif val.HasField('int_value'):
                    properties[name] = val.int_value
                elif val.HasField('float_value'):
                    properties[name] = val.float_value
                elif val.HasField('double_value'):
                    properties[name] = val.double_value
                else:
                    raise Exception("Unknown value type: '%s'" % val)
            fobj['properties'] = properties
            x,y = self._decode_coords(feat.geometry[1],feat.geometry[2])
            if lonlat:
                x,y = merc2lonlat(x,y)
            fobj['geometry'] = {
                "type":"Point",
                "coordinates": [x,y]
            }
            features.append(fobj)
        jobj['features'] = features
        return json.dumps(jobj,indent=4)

    def _handle_attr(self, layer, feature, props):
      for k,v in props.items():
          if k not in self.keys:
              layer.keys.append(k)
              self.keys.append(k)
              idx = self.keys.index(k)
              feature.tags.append(idx)
          else:
              idx = self.keys.index(k)
              feature.tags.append(idx)
          if v not in self.values:
              if (isinstance(v,bool)):
                  val = layer.values.add()
                  val.bool_value = v
              elif (isinstance(v,str)) or (isinstance(v,unicode)):
                  val = layer.values.add()
                  val.string_value = v
              elif (isinstance(v,int)):
                  val = layer.values.add()
                  val.int_value = v
              elif (isinstance(v,float)):
                  val = layer.values.add()
                  val.double_value = v
              else:
                  raise Exception("Unknown value type: '%s'" % type(v))
              self.values.append(v)
              feature.tags.append(self.values.index(v))
