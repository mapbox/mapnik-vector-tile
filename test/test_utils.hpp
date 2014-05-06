// mapnik
#include "mapnik3x_compatibility.hpp"
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/save_map.hpp>
#include <mapnik/map.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/memory_datasource.hpp>

// boost
#include MAPNIK_SHARED_INCLUDE
#include MAPNIK_MAKE_SHARED_INCLUDE

#include <string>

MAPNIK_SHARED_PTR<mapnik::memory_datasource> build_ds(double x,double y) {
    mapnik::context_ptr ctx = MAPNIK_MAKE_SHARED<mapnik::context_type>();
    ctx->push("name");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::transcoder tr("utf-8");
    feature->put("name",tr.transcode("null island"));
    mapnik::geometry_type * pt = new mapnik::geometry_type(MAPNIK_POINT);
    pt->move_to(x,y);
    feature->add_geometry(pt);
    MAPNIK_SHARED_PTR<mapnik::memory_datasource> ds = MAPNIK_MAKE_SHARED<mapnik::memory_datasource>();
    ds->push(feature);
    return ds;
}
