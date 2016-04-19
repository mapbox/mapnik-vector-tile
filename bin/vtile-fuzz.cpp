#include "vector_tile_geometry_clipper.hpp"

#include <vector>
#include <iostream>
#include <limits>
#include <cstdlib>


/*

https://github.com/mapnik/clipper/issues/3

*/

class noop_process {
public:
    noop_process() {}
    template <typename T>
    void operator() (T &)
    {
        return;
    }
};

int main() {

    noop_process no_op;
    using clipping_process = mapnik::vector_tile_impl::geometry_clipper<noop_process>;
    mapnik::box2d<int> no_op_max_extent(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max(),
                                            std::numeric_limits<int>::max());
    while (1)
    {

        std::size_t len = std::rand() % 700 + 3;

        for (auto strictly_simple : { true, false })
        {
            for (auto multi_polygon_union : { true, false })
            {
                for (auto process_all_rings : { true, false })
                {
                    for (auto area_threshold : { 0.0, 0.5 })
                    {
                        for (auto fill_type : { mapnik::vector_tile_impl::even_odd_fill,
                                                mapnik::vector_tile_impl::non_zero_fill,
                                                mapnik::vector_tile_impl::positive_fill,
                                                mapnik::vector_tile_impl::negative_fill })
                        {
                            mapnik::geometry::polygon<std::int64_t> poly;
                            mapnik::geometry::linear_ring<std::int64_t> ring;
                            mapnik::box2d<int> geom_extent;

                            bool added_exterior = false;
                            bool first = true;

                            for (std::size_t i = 0; i < len; ++i) {
                                std::int64_t x = std::rand() % 4096;
                                std::int64_t y = std::rand() % 4096;

                                ring.add_coord(x,y);
                                if (first) {
                                    first = false;
                                    geom_extent.init(x,y,x,y);
                                } else {
                                    geom_extent.expand_to_include(x,y);
                                }

                                if (!added_exterior && i > 100) {
                                    poly.set_exterior_ring(std::move(ring));
                                    ring = mapnik::geometry::linear_ring<std::int64_t>();
                                    added_exterior = true;
                                }

                                if (added_exterior && std::rand() % 50 == 0 && ring.size() >= 3) {
                                    poly.add_hole(std::move(ring));
                                    ring = mapnik::geometry::linear_ring<std::int64_t>();
                                }
                            }
                            std::clog << "size: " << poly.num_rings() << "\n";
                            // clip with max extent: should result in nothing being clipped
                            {
                                clipping_process clipper(no_op_max_extent,
                                                         area_threshold,
                                                         strictly_simple,
                                                         multi_polygon_union,
                                                         fill_type,
                                                         process_all_rings,
                                                         no_op);
                                clipper(poly);
                            }
                            // clip with the extent of geometries minus a bit
                            {
                                // TODO - mapnik core needs support for ints
                                //mapnik::box2d<int> geom_extent = mapnik::geometry::envelope(poly);
                                geom_extent.pad(-100);
                                clipping_process clipper(geom_extent,
                                                         area_threshold,
                                                         strictly_simple,
                                                         multi_polygon_union,
                                                         fill_type,
                                                         process_all_rings,
                                                         no_op);
                                clipper(poly);
                            }
                        }
                    }
                }
            }
        }
    }
}