#ifndef __MAPNIK_VTILE_COMPARE_IMAGES_HPP__
#define __MAPNIK_VTILE_COMPARE_IMAGES_HPP__ 

#include "mapnik3x_compatibility.hpp"
#include MAPNIK_SHARED_INCLUDE
#include MAPNIK_MAKE_SHARED_INCLUDE
#include <mapnik/graphics.hpp>
#include <mapnik/image_data.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/image_reader.hpp>

using namespace mapnik;

namespace testing {

    unsigned compare_images(image_data_32 const& src1,
                            image_data_32 const& src2,
                            int threshold=16,
                            bool alpha=true)
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
                            int threshold=16,
                            bool alpha=true)
    {
        boost::optional<std::string> type = mapnik::type_from_filename(dest_fn);
        if (!type)
        {
            throw mapnik::image_reader_exception("Failed to detect type of: " + dest_fn);
        }
        MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader1(mapnik::get_image_reader(dest_fn,*type));
        if (!reader1.get())
        {
            throw mapnik::image_reader_exception("Failed to load: " + dest_fn);
        }
        MAPNIK_SHARED_PTR<image_32> image_ptr1 = MAPNIK_MAKE_SHARED<image_32>(reader1->width(),reader1->height());
        reader1->read(0,0,image_ptr1->data());

        boost::optional<std::string> type2 = mapnik::type_from_filename(src_fn);
        if (!type2)
        {
            throw mapnik::image_reader_exception("Failed to detect type of: " + src_fn);
        }
        MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader2(mapnik::get_image_reader(src_fn,*type2));
        if (!reader2.get())
        {
            throw mapnik::image_reader_exception("Failed to load: " + src_fn);
        }
        MAPNIK_SHARED_PTR<image_32> image_ptr2 = MAPNIK_MAKE_SHARED<image_32>(reader2->width(),reader2->height());
        reader2->read(0,0,image_ptr2->data());

        image_data_32 const& src1 = image_ptr1->data();
        image_data_32 const& src2 = image_ptr2->data();
        return compare_images(src1,src2);
    }

    unsigned compare_images(image_data_32 const& src1,
                            std::string const& filepath,
                            int threshold=16,
                            bool alpha=true)
    {
        boost::optional<std::string> type = mapnik::type_from_filename(filepath);
        if (!type)
        {
            throw mapnik::image_reader_exception("Failed to detect type of: " + filepath);
        }
        MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader2(mapnik::get_image_reader(filepath,*type));
        if (!reader2.get())
        {
            throw mapnik::image_reader_exception("Failed to load: " + filepath);
        }
        MAPNIK_SHARED_PTR<image_32> image_ptr2 = MAPNIK_MAKE_SHARED<image_32>(reader2->width(),reader2->height());
        reader2->read(0,0,image_ptr2->data());

        image_data_32 const& src2 = image_ptr2->data();
        return compare_images(src1,src2);
    }

}

#endif // __MAPNIK_VTILE_COMPARE_IMAGES_HPP__
