// mapnik
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

#include <string>
#include <memory>

std::shared_ptr<mapnik::memory_datasource> build_ds(double x,double y, bool second=false) {
    mapnik::parameters params;
    params["type"] = "memory";
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(params);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    ctx->push("name");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::transcoder tr("utf-8");
    feature->put("name",tr.transcode("null island"));
    mapnik::geometry_type * pt = new mapnik::geometry_type(mapnik::geometry_type::types::Point);
    pt->move_to(x,y);
    feature->add_geometry(pt);
    ds->push(feature);
    if (second) {
        ctx->push("name2");
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
        mapnik::transcoder tr("utf-8");
        feature->put("name",tr.transcode("null island"));
        feature->put("name2",tr.transcode("null island 2"));
        mapnik::geometry_type * pt = new mapnik::geometry_type(mapnik::geometry_type::types::Point);
        pt->move_to(x+1,y+1);
        feature->add_geometry(pt);
        ds->push(feature);
    }
    return ds;
}
