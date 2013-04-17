#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import json
# put local python module dir on path
sys.path.append("./python")

import renderer

class TestRendererParts(unittest.TestCase):
    def test_lonlat2merc(self):
        # projection transform
        # should roughtly match: echo -180 -85 | cs2cs -f "%.10f" +init=epsg:4326 +to +init=epsg:3857
        x,y = renderer.lonlat2merc(-180,-85)
        self.assertAlmostEqual(-20037508.342789244,x)
        self.assertAlmostEqual(-19971868.8804085888,y)
    
    def test_box2d(self):
        box = renderer.Box2d(-180,-85,180,85)
        assert box.minx == -180
        assert box.miny == -85
        assert box.maxx == 180
        assert box.maxy == 85
        assert box.intersects(0,0)
        assert not box.intersects(-180,-90)
        self.assertAlmostEqual(box.bounds(),[box.minx,box.miny,box.maxx,box.maxy])
        
    def test_spherical_mercator(self):
        merc = renderer.SphericalMercator()
        z0_extent = merc.bbox(0,0,0)
        
    def test_request(self):
        req = renderer.Request(0,0,0)
        self.assertAlmostEqual(req.get_width(),40075016.68557849)
        self.assertAlmostEqual(req.get_height(),40075016.68557849)

    def test_ctrans(self):
        req = renderer.Request(0,0,0)
        x,y = renderer.lonlat2merc(-180,-85)
        ctrans = renderer.CoordTransform(req)
        px,py = ctrans.forward(x,y)
        self.assertAlmostEqual(px,0.0)
        self.assertAlmostEqual(py,255.5806938147701)
        px2,py2 = ctrans.forward(-20037508.34,-20037508.34)
        self.assertAlmostEqual(px2,0.0)
        self.assertAlmostEqual(py2,256.0)
        px3,py3 = ctrans.forward(-20037508.34/2,-20037508.34/2)
        self.assertAlmostEqual(px2,0.0)
        self.assertAlmostEqual(py2,256.0)
    
    def test_adding_duplicate_points(self):
        req = renderer.Request(0,0,0)
        vtile = renderer.VectorTile(req)
        vtile.add_point(0,0,{})
        vtile.add_point(0,0,{})
        j_obj = json.loads(vtile.to_geojson())
        self.assertEqual(len(j_obj['features']),1)

    def test_vtile_attributes(self):
        req = renderer.Request(0,0,0)
        vtile = renderer.VectorTile(req)
        attr = {"name":"DC",
                "integer":10,
                "bigint":sys.maxint,
                "nbigint":-1 * sys.maxint,
                "float":1.5,
                "bigfloat":float(sys.maxint),
                "unistr":u"élan",
                "bool":True,
                "bool2":False
                }
        vtile.add_point(0,0,attr)
        j_obj = json.loads(vtile.to_geojson())
        self.assertEqual(j_obj['type'],"FeatureCollection")
        self.assertEqual(len(j_obj['features']),1)
        feature = j_obj['features'][0]
        self.assertDictEqual(feature['properties'],attr)
    
    def test_vtile_z0(self):
        req = renderer.Request(0,0,0)
        vtile = renderer.VectorTile(req)
        x,y = -8526703.378081053, 4740318.745473632
        vtile.add_point(x,y,{})
        j_obj = json.loads(vtile.to_geojson())
        feature = j_obj['features'][0]
        self.assertEqual(feature['type'],'Feature')
        self.assertEqual(feature['geometry']['type'],'Point')
        coords = feature['geometry']['coordinates']
        self.assertAlmostEqual(coords[0],x,-4)
        self.assertAlmostEqual(coords[1],y,-4)

    def test_vtile_z20(self):
        merc = renderer.SphericalMercator()
        x,y = -8526703.378081053, 4740318.745473632
        xyz_bounds = merc.xyz([x,y,x,y],20)
        req = renderer.Request(xyz_bounds[0],xyz_bounds[1],20)
        vtile = renderer.VectorTile(req)
        vtile.add_point(x,y,{"name":"DC","integer":10,"float":1.5})
        j_obj = json.loads(vtile.to_geojson())
        self.assertEqual(j_obj['type'],"FeatureCollection")
        self.assertEqual(len(j_obj['features']),1)
        feature = j_obj['features'][0]
        self.assertDictEqual(feature['properties'],{ "integer": 10, 
                                                     "float": 1.5, 
                                                     "name": "DC"
                                                   })
        self.assertEqual(feature['type'],'Feature')
        self.assertEqual(feature['geometry']['type'],'Point')
        coords = feature['geometry']['coordinates']
        self.assertAlmostEqual(coords[0],x,2)
        self.assertAlmostEqual(coords[1],y,2)

    def test_vtile_z20_higher_precision(self):
        merc = renderer.SphericalMercator()
        x,y = -8526703.378081053, 4740318.745473632
        xyz_bounds = merc.xyz([x,y,x,y],20)
        req = renderer.Request(xyz_bounds[0],xyz_bounds[1],20)
        vtile = renderer.VectorTile(req,512)
        vtile.add_point(x,y,{})
        j_obj = json.loads(vtile.to_geojson())
        feature = j_obj['features'][0]
        coords = feature['geometry']['coordinates']
        self.assertAlmostEqual(coords[0],x,3)
        self.assertAlmostEqual(coords[1],y,3)

    def test_vtile_z22(self):
        merc = renderer.SphericalMercator()
        x,y = -8526703.378081053, 4740318.745473632
        xyz_bounds = merc.xyz([x,y,x,y],22)
        req = renderer.Request(xyz_bounds[0],xyz_bounds[1],22)
        vtile = renderer.VectorTile(req)
        vtile.add_point(x,y,{"name":"DC","integer":10,"float":1.5})
        j_obj = json.loads(vtile.to_geojson())
        self.assertEqual(j_obj['type'],"FeatureCollection")
        self.assertEqual(len(j_obj['features']),1)
        feature = j_obj['features'][0]
        self.assertDictEqual(feature['properties'],{ "integer": 10, 
                                                     "float": 1.5, 
                                                     "name": "DC"
                                                   })
        self.assertEqual(feature['type'],'Feature')
        self.assertEqual(feature['geometry']['type'],'Point')
        coords = feature['geometry']['coordinates']
        self.assertAlmostEqual(coords[0],x,2)
        self.assertAlmostEqual(coords[1],y,1)

    def test_vtile_z22_higher_precision(self):
        merc = renderer.SphericalMercator()
        x,y = -8526703.378081053, 4740318.745473632
        xyz_bounds = merc.xyz([x,y,x,y],22)
        req = renderer.Request(xyz_bounds[0],xyz_bounds[1],22)
        vtile = renderer.VectorTile(req,512)
        vtile.add_point(x,y,{})
        j_obj = json.loads(vtile.to_geojson())
        feature = j_obj['features'][0]
        coords = feature['geometry']['coordinates']
        self.assertAlmostEqual(coords[0],x,4)
        self.assertAlmostEqual(coords[1],y,4)

if __name__ == '__main__':
    unittest.main()