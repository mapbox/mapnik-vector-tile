// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/image_util.hpp>
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
#include <mapnik/image.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/json/geometry_parser.hpp>
#include <string>
#include <memory>

namespace testing {

std::shared_ptr<mapnik::memory_datasource> build_ds(double x,double y, bool second) {
    mapnik::parameters params;
    params["type"] = "memory";
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(params);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    ctx->push("name");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::transcoder tr("utf-8");
    feature->put("name",tr.transcode("null island"));
    // NOTE: all types below that are not part of mapnik::value
    // are likely getting converted, e.g. float -> double
    feature->put_new("int",static_cast<int64_t>(-73));
    feature->put_new("uint",static_cast<uint64_t>(37));
    feature->put_new("float",static_cast<float>(99.2));
    feature->put_new("double",static_cast<double>(83.4));
    feature->put_new("bool",true);
    feature->put_new("boolf",false);
    feature->set_geometry(mapnik::geometry::point<double>(x,y));
    ds->push(feature);
    if (second) {
        ctx->push("name2");
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
        mapnik::transcoder tr("utf-8");
        feature->put("name",tr.transcode("null island"));
        feature->put("name2",tr.transcode("null island 2"));
        feature->set_geometry(mapnik::geometry::point<double>(x+1,y+1));
        ds->push(feature);
    }
    return ds;
}

std::shared_ptr<mapnik::memory_datasource> build_geojson_ds(std::string const& geojson_file) {
    mapnik::util::file input(geojson_file);
    if (!input.open())
    {
        throw std::runtime_error("failed to open geojson");
    }
    mapnik::geometry::geometry<double> geom;
    std::string json_string(input.data().get(), input.size());
    if (!mapnik::json::from_geojson(json_string, geom))
    {
        throw std::runtime_error("failed to parse geojson");
    }
    mapnik::geometry::correct(geom);
    mapnik::parameters params;
    params["type"] = "memory";
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(params);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    ctx->push("name");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    feature->set_geometry(std::move(geom));
    ds->push(feature);
    return ds;
}

unsigned compare_images(mapnik::image_rgba8 const& src1,
                        std::string const& filepath,
                        int threshold,
                        bool alpha)
{
    boost::optional<std::string> type = mapnik::type_from_filename(filepath);
    if (!type)
    {
        throw mapnik::image_reader_exception("Failed to detect type of: " + filepath);
    }
    std::unique_ptr<mapnik::image_reader> reader2(mapnik::get_image_reader(filepath,*type));
    if (!reader2.get())
    {
        throw mapnik::image_reader_exception("Failed to load: " + filepath);
    }
    mapnik::image_any const& image_2 = reader2->read(0,0,reader2->width(),reader2->height());

    mapnik::image_rgba8 const& src2 = mapnik::util::get<mapnik::image_rgba8>(image_2);
    return mapnik::compare(src1,src2,threshold,alpha);
}


} // end ns
