#include "catch.hpp"

// mapnik-vector-tile
#include "vector_tile_compression.hpp"

TEST_CASE("invalid decompression")
{
    std::string data("this is a string that should be compressed data");
    // data is not compressed but we will try to decompress it
    std::string output;
    CHECK_THROWS(mapnik::vector_tile_impl::zlib_decompress(data, output));
}

TEST_CASE("round trip compression - zlib")
{
    std::string data("this is a sentence that will be compressed into something");
    CHECK(!mapnik::vector_tile_impl::is_zlib_compressed(data));
    
    int strategy;

    SECTION("strategy - invalid compression")
    {
        strategy = 99;
        int level = Z_DEFAULT_COMPRESSION;
        std::string compressed_data;
        CHECK_THROWS(mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy));
    }
    
    SECTION("compression level - invalid")
    {
        strategy = Z_DEFAULT_STRATEGY;
        int level = 99;
        std::string compressed_data;
        CHECK_THROWS(mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy));
    }

    SECTION("strategy - default")
    {
        strategy = Z_DEFAULT_STRATEGY;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - filtered")
    {
        strategy = Z_FILTERED;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - huffman only")
    {
        strategy = Z_HUFFMAN_ONLY;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - rle")
    {
        strategy = Z_RLE;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - fixed")
    {
        strategy = Z_FIXED;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, false, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_zlib_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }
}

TEST_CASE("round trip compression - gzip")
{
    std::string data("this is a sentence that will be compressed into something");
    CHECK(!mapnik::vector_tile_impl::is_gzip_compressed(data));
    
    int strategy;

    SECTION("strategy - invalid compression")
    {
        strategy = 99;
        int level = Z_DEFAULT_COMPRESSION;
        std::string compressed_data;
        CHECK_THROWS(mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy));
    }

    SECTION("compression level - invalid")
    {
        strategy = Z_DEFAULT_STRATEGY;
        int level = 99;
        std::string compressed_data;
        CHECK_THROWS(mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy));
    }

    SECTION("strategy - default")
    {
        strategy = Z_DEFAULT_STRATEGY;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - filtered")
    {
        strategy = Z_FILTERED;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - huffman only")
    {
        strategy = Z_HUFFMAN_ONLY;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - rle")
    {
        strategy = Z_RLE;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }

    SECTION("strategy - fixed")
    {
        strategy = Z_FIXED;

        SECTION("no compression")
        {
            int level = Z_NO_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("default compression level")
        {
            int level = Z_DEFAULT_COMPRESSION;
            std::string compressed_data;
            mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
            CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
            std::string new_data;
            mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
            CHECK(data == new_data);
        }

        SECTION("compression level -- min to max")
        {
            for (int level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; ++level)
            {
                std::string compressed_data;
                mapnik::vector_tile_impl::zlib_compress(data, compressed_data, true, level, strategy);
                CHECK(mapnik::vector_tile_impl::is_gzip_compressed(compressed_data));
                std::string new_data;
                mapnik::vector_tile_impl::zlib_decompress(compressed_data, new_data);
                CHECK(data == new_data);
            }
        }
    }
}
