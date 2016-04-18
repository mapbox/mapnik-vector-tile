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
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::even_odd_fill;
    mapnik::box2d<int> tile_clipping_extent(std::numeric_limits<int>::min(),
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

                        clipping_process clipper(tile_clipping_extent,
                                                 area_threshold,
                                                 strictly_simple,
                                                 multi_polygon_union,
                                                 fill_type,
                                                 process_all_rings,
                                                 no_op);

                        mapnik::geometry::polygon<std::int64_t> poly;
                        mapnik::geometry::linear_ring<std::int64_t> ring;

                        bool added_exterior = false;

                        for (std::size_t i = 0; i < len; ++i) {
                            std::int64_t x = std::rand() % 4096;
                            std::int64_t y = std::rand() % 4096;

                            ring.add_coord(x,y);

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
                        clipper(poly);
                    }
                }
            }
        }
    }
}