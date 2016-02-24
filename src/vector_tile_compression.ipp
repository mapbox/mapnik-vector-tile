// zlib
#include <zlib.h>

// std
#include <stdexcept>

namespace mapnik
{

namespace vector_tile_impl 
{

// decodes both zlib and gzip
// http://stackoverflow.com/a/1838702/2333354
void zlib_decompress(const char * data, std::size_t size, std::string & output)
{
    z_stream inflate_s;
    inflate_s.zalloc = Z_NULL;
    inflate_s.zfree = Z_NULL;
    inflate_s.opaque = Z_NULL;
    inflate_s.avail_in = 0;
    inflate_s.next_in = Z_NULL;
    inflateInit2(&inflate_s, 32 + 15);
    inflate_s.next_in = (Bytef *)data;
    inflate_s.avail_in = size;
    size_t length = 0;
    do {
        output.resize(length + 2 * size);
        inflate_s.avail_out = 2 * size;
        inflate_s.next_out = (Bytef *)(output.data() + length);
        int ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            std::string error_msg = inflate_s.msg;
            inflateEnd(&inflate_s);
            throw std::runtime_error(error_msg);
        }

        length += (2 * size - inflate_s.avail_out);
    } while (inflate_s.avail_out == 0);
    inflateEnd(&inflate_s);
    output.resize(length);
}

void zlib_decompress(std::string const& input, std::string & output)
{
    zlib_decompress(input.data(),input.size(),output);
}

void zlib_compress(const char * data, std::size_t size, std::string & output, bool gzip, int level, int strategy)
{
    z_stream deflate_s;
    deflate_s.zalloc = Z_NULL;
    deflate_s.zfree = Z_NULL;
    deflate_s.opaque = Z_NULL;
    deflate_s.avail_in = 0;
    deflate_s.next_in = Z_NULL;
    int windowsBits = 15;
    if (gzip)
    {
        windowsBits = windowsBits | 16;
    }
    if (deflateInit2(&deflate_s, level, Z_DEFLATED, windowsBits, 8, strategy) != Z_OK)
    {
        throw std::runtime_error("deflate init failed");
    }
    deflate_s.next_in = (Bytef *)data;
    deflate_s.avail_in = size;
    size_t length = 0;
    do {
        size_t increase = size / 2 + 1024;
        output.resize(length + increase);
        deflate_s.avail_out = increase;
        deflate_s.next_out = (Bytef *)(output.data() + length);
        // From http://www.zlib.net/zlib_how.html
        // "deflate() has a return value that can indicate errors, yet we do not check it here. 
        // Why not? Well, it turns out that deflate() can do no wrong here."
        // Basically only possible error is from deflateInit not working properly
        deflate(&deflate_s, Z_FINISH);
        length += (increase - deflate_s.avail_out);
    } while (deflate_s.avail_out == 0);
    deflateEnd(&deflate_s);
    output.resize(length);
}

void zlib_compress(std::string const& input, std::string & output, bool gzip, int level, int strategy)
{
    zlib_compress(input.data(),input.size(),output,gzip,level,strategy);
}

} // end ns vector_tile_impl

} // end ns mapnik
