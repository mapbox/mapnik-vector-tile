#include <stdexcept>
#include <zlib.h>

namespace mapnik { namespace vector_tile_impl {

// decodes both zlib and gzip
// http://stackoverflow.com/a/1838702/2333354
void zlib_decompress(std::string const& input, std::string & output)
{
    z_stream inflate_s;
    inflate_s.zalloc = Z_NULL;
    inflate_s.zfree = Z_NULL;
    inflate_s.opaque = Z_NULL;
    inflate_s.avail_in = 0;
    inflate_s.next_in = Z_NULL;
    inflateInit2(&inflate_s, 32 + 15);
    inflate_s.next_in = (Bytef *)input.data();
    inflate_s.avail_in = input.size();
    size_t length = 0;
    do {
        output.resize(length + 2 * input.size());
        inflate_s.avail_out = 2 * input.size();
        inflate_s.next_out = (Bytef *)(output.data() + length);
        int ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
            throw std::runtime_error(inflate_s.msg);
        }

        length += (2 * input.size() - inflate_s.avail_out);
    } while (inflate_s.avail_out == 0);
    inflateEnd(&inflate_s);
    output.resize(length);
}

void zlib_compress(std::string const& input, std::string & output, bool gzip)
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
    if (deflateInit2(&deflate_s, Z_BEST_COMPRESSION, Z_DEFLATED, windowsBits, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        throw std::runtime_error("deflate failed");
    }
    deflate_s.next_in = (Bytef *)input.data();
    deflate_s.avail_in = input.size();
    size_t length = 0;
    do {
        size_t increase = input.size() / 2 + 1024;
        output.resize(length + increase);
        deflate_s.avail_out = increase;
        deflate_s.next_out = (Bytef *)(output.data() + length);
        int ret = deflate(&deflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
            throw std::runtime_error(deflate_s.msg);
        }
        length += (increase - deflate_s.avail_out);
    } while (deflate_s.avail_out == 0);
    deflateEnd(&deflate_s);
    output.resize(length);
}

}}
