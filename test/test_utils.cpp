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
#include <mapnik/graphics.hpp>
#include <mapnik/image_data.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/image_reader.hpp>

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

unsigned compare_images(mapnik::image_data_rgba8 const& src1,
                        mapnik::image_data_rgba8 const& src2,
                        int threshold,
                        bool alpha)
{
    unsigned difference = 0;
    unsigned int width = src1.width();
    unsigned int height = src1.height();
    if ((width != src2.width()) || height != src2.height()) return false;
    for (unsigned int y = 0; y < height; ++y)
    {
        const unsigned int* row_from = src1.getRow(y);
        const unsigned int* row_from2 = src2.getRow(y);
        for (unsigned int x = 0; x < width; ++x)
        {
            unsigned rgba = row_from[x];
            unsigned rgba2 = row_from2[x];
            unsigned r = rgba & 0xff;
            unsigned g = (rgba >> 8 ) & 0xff;
            unsigned b = (rgba >> 16) & 0xff;
            unsigned r2 = rgba2 & 0xff;
            unsigned g2 = (rgba2 >> 8 ) & 0xff;
            unsigned b2 = (rgba2 >> 16) & 0xff;
            if (std::abs(static_cast<int>(r - r2)) > threshold ||
                std::abs(static_cast<int>(g - g2)) > threshold ||
                std::abs(static_cast<int>(b - b2)) > threshold) {
                ++difference;
                continue;
            }
            if (alpha) {
                unsigned a = (rgba >> 24) & 0xff;
                unsigned a2 = (rgba2 >> 24) & 0xff;
                if (std::abs(static_cast<int>(a - a2)) > threshold) {
                    ++difference;
                    continue;
                }
            }
        }
    }
    return difference;
}

unsigned compare_images(std::string const& src_fn,
                        std::string const& dest_fn,
                        int threshold,
                        bool alpha)
{
    boost::optional<std::string> type = mapnik::type_from_filename(dest_fn);
    if (!type)
    {
        throw mapnik::image_reader_exception("Failed to detect type of: " + dest_fn);
    }
    std::unique_ptr<mapnik::image_reader> reader1(mapnik::get_image_reader(dest_fn,*type));
    if (!reader1.get())
    {
        throw mapnik::image_reader_exception("Failed to load: " + dest_fn);
    }
    std::shared_ptr<mapnik::image_32> image_ptr1 = std::make_shared<mapnik::image_32>(reader1->width(),reader1->height());
    reader1->read(0,0,image_ptr1->data());

    boost::optional<std::string> type2 = mapnik::type_from_filename(src_fn);
    if (!type2)
    {
        throw mapnik::image_reader_exception("Failed to detect type of: " + src_fn);
    }
    std::unique_ptr<mapnik::image_reader> reader2(mapnik::get_image_reader(src_fn,*type2));
    if (!reader2.get())
    {
        throw mapnik::image_reader_exception("Failed to load: " + src_fn);
    }
    std::shared_ptr<mapnik::image_32> image_ptr2 = std::make_shared<mapnik::image_32>(reader2->width(),reader2->height());
    reader2->read(0,0,image_ptr2->data());

    mapnik::image_data_rgba8 const& src1 = image_ptr1->data();
    mapnik::image_data_rgba8 const& src2 = image_ptr2->data();
    return compare_images(src1,src2,threshold,alpha);
}

unsigned compare_images(mapnik::image_data_rgba8 const& src1,
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
    std::shared_ptr<mapnik::image_32> image_ptr2 = std::make_shared<mapnik::image_32>(reader2->width(),reader2->height());
    reader2->read(0,0,image_ptr2->data());

    mapnik::image_data_rgba8 const& src2 = image_ptr2->data();
    return compare_images(src1,src2,threshold,alpha);
}

} // end ns
