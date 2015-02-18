#ifndef __MAPNIK_VECTOR_TEST_UTILS_H__
#define __MAPNIK_VECTOR_TEST_UTILS_H__

// mapnik
#include <mapnik/memory_datasource.hpp>
#include <mapnik/image_data.hpp>
#include <memory>

namespace testing {

std::shared_ptr<mapnik::memory_datasource> build_ds(double x,double y, bool second=false);

unsigned compare_images(mapnik::image_data_rgba8 const& src1,
                        mapnik::image_data_rgba8 const& src2,
                        int threshold=16,
                        bool alpha=true);
unsigned compare_images(std::string const& src_fn,
                        std::string const& dest_fn,
                        int threshold=16,
                        bool alpha=true);
unsigned compare_images(mapnik::image_data_rgba8 const& src1,
                        std::string const& filepath,
                        int threshold=16,
                        bool alpha=true);

}

#endif // __MAPNIK_VECTOR_TEST_UTILS_H__