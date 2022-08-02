/**
 * Deflate to same buffer.
 *
 * Perhaps obvious, but same buffer compression is mostly interesting if the source buffer is already
 * in memory and you want to save some space compressing it.
 * So, this example reads the entire file into a std::string, then that same std::string is
 * then compressed into itself, and finally truncated to it's smaller size.
 *
 * This SO response, by Mark Adler, https://stackoverflow.com/a/12412863, discusses the theory that
 * some data might not compress enough to ensure that the compressed data NEVER overlaps what is not
 * yet read. I have not taken any of this into account so this may still be a thing, but the only
 * way I could see a problem was compressing already compressed data, but still not for every test.
 *
 * However I have tested it with various Text files and binary files such as 15GB disk images,
 * without fail. I have not made many tests with other compression formats.
 *
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <zlib.hpp>

namespace ot = om_tools;

void same_buffer_compress(std::string& data) {
    ot::same_string_writer same_writer(data);
    ot::zlib_deflator<ot::same_string_writer>(same_writer)
        .deflate(&data[0], data.size());

    // truncate the string to it's hopefully smaller size
    // the same_string_writer knows how much was written.
    same_writer.truncate();
}

std::string decompress(const std::string& data) {
    std::string result;
    ot::string_writer writer(result);
    ot::zlib_inflator<ot::string_writer>(writer)
        .inflate(&data[0], data.size());
    return result;
}

int main(int argc, char **argv) {
    std::vector<std::string> params(argv, argv + argc);

    std::string filename;
    if (params.size() == 1) {
        filename = "Makefile";
    } else {
        filename = params.at(1);
    }

    // read the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::clog << "Cannot open file: " << filename << '\n';
        return 1;
    }
    if (ot::same_string_writer::maybe_gzip_deflated(file)) {
        std::clog << "Looks like this is already be gzip/deflated\n";
        return 1;
    }

    std::stringstream ss; ss << file.rdbuf();
    std::string compressed_content = ss.str();
    size_t original_size = compressed_content.size();
    std::string original_content = compressed_content;

    same_buffer_compress(compressed_content);

    size_t compressed_size = compressed_content.size();
    std::string decompressed = decompress(compressed_content);
    size_t decompressed_size = decompressed.size();

    // verify
    if (original_content.size() == decompressed.size()) {
        std::string::iterator orig_it = original_content.begin();
        std::string::iterator decom_it = decompressed.begin();
        for (; orig_it != original_content.end(); ++orig_it, ++decom_it) {
            assert(*orig_it == *decom_it);
        }
    }

    std::cout
        << "Successfully compressed to same buffer\n"
        << "Original size: " << original_size << '\n'
        << "Compressed size: " << compressed_size << '\n'
        << "Decompressed size: " << decompressed_size << '\n';
    return 0;
}


// this is the function suggested in https://stackoverflow.com/a/12412863

/* Compress buf[0..len-1] in place into buf[0..*max-1].  *max must be greater
   than or equal to len.  Return Z_OK on success, Z_BUF_ERROR if *max is not
   enough output space, Z_MEM_ERROR if there is not enough memory, or
   Z_STREAM_ERROR if *strm is corrupted (e.g. if it wasn't initialized or if it
   was inadvertently written over).  If Z_OK is returned, *max is set to the
   actual size of the output.  If Z_BUF_ERROR is returned, then *max is
   unchanged and buf[] is filled with *max bytes of uncompressed data (which is
   not all of it, but as much as would fit).

   Incompressible data will require more output space than len, so max should
   be sufficiently greater than len to handle that case in order to avoid a
   Z_BUF_ERROR. To assure that there is enough output space, max should be
   greater than or equal to the result of deflateBound(strm, len).

   strm is a deflate stream structure that has already been successfully
   initialized by deflateInit() or deflateInit2().  That structure can be
   reused across multiple calls to deflate_inplace().  This avoids unnecessary
   memory allocations and deallocations from the repeated use of deflateInit()
   and deflateEnd(). */
int deflate_inplace(z_stream *strm, unsigned char *buf, unsigned len,
                    unsigned *max)
{
    int ret;                    /* return code from deflate functions */
    unsigned have;              /* number of bytes in temp[] */
    unsigned char *hold;        /* allocated buffer to hold input data */
    unsigned char temp[11];     /* must be large enough to hold zlib or gzip
                                   header (if any) and one more byte -- 11
                                   works for the worst case here, but if gzip
                                   encoding is used and a deflateSetHeader()
                                   call is inserted in this code after the
                                   deflateReset(), then the 11 needs to be
                                   increased to accomodate the resulting gzip
                                   header size plus one */

    /* initialize deflate stream and point to the input data */
    ret = deflateReset(strm);
    if (ret != Z_OK)
        return ret;
    strm->next_in = buf;
    strm->avail_in = len;

    /* kick start the process with a temporary output buffer -- this allows
       deflate to consume a large chunk of input data in order to make room for
       output data there */
    if (*max < len)
        *max = len;
    strm->next_out = temp;
    strm->avail_out = sizeof(temp) > *max ? *max : sizeof(temp);
    ret = deflate(strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR)
        return ret;

    /* if we can, copy the temporary output data to the consumed portion of the
       input buffer, and then continue to write up to the start of the consumed
       input for as long as possible */
    have = strm->next_out - temp;
    if (have <= (strm->avail_in ? len - strm->avail_in : *max)) {
        memcpy(buf, temp, have);
        strm->next_out = buf + have;
        have = 0;
        while (ret == Z_OK) {
            strm->avail_out = strm->avail_in ? strm->next_in - strm->next_out :
                              (buf + *max) - strm->next_out;
            ret = deflate(strm, Z_FINISH);
        }
        if (ret != Z_BUF_ERROR || strm->avail_in == 0) {
            *max = strm->next_out - buf;
            return ret == Z_STREAM_END ? Z_OK : ret;
        }
    }

    /* the output caught up with the input due to insufficiently compressible
       data -- copy the remaining input data into an allocated buffer and
       complete the compression from there to the now empty input buffer (this
       will only occur for long incompressible streams, more than ~20 MB for
       the default deflate memLevel of 8, or when *max is too small and less
       than the length of the header plus one byte) */
    hold = static_cast<unsigned char *>(strm->zalloc(strm->opaque, strm->avail_in, 1));
    if (hold == Z_NULL)
        return Z_MEM_ERROR;
    memcpy(hold, strm->next_in, strm->avail_in);
    strm->next_in = hold;
    if (have) {
        memcpy(buf, temp, have);
        strm->next_out = buf + have;
    }
    strm->avail_out = (buf + *max) - strm->next_out;
    ret = deflate(strm, Z_FINISH);
    strm->zfree(strm->opaque, hold);
    *max = strm->next_out - buf;
    return ret == Z_OK ? Z_BUF_ERROR : (ret == Z_STREAM_END ? Z_OK : ret);
}
