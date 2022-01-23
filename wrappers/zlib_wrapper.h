#pragma once

/*
 * zlib_helper.h
 *
 * Easy to use incremental compression based on the fine
 * https://zlib.net/zlib_how.html zlib usage example
 *
 * Output can be anything that stores character data with a write function,
 * see string_writer for requirement. std::ofstream for example does just that
 *
 * Functions accepting a "const char*" casts the const away since zlib uses
 * "unsigned char*" in it's buffers (so the char* being signed needs
 * reinterpret_casting too)
 * Here is a question in the matter with an explanation from one of the authors of zlib:
 * https://stackoverflow.com/questions/44958875/c-using-zlib-with-const-data
 *
 * An example function showing from string to string, and from file to file:

void example() {
    using namespace sls_std;
    const char deflate_this[] = "some text that takes up too much space in the world, there is a lot of text around tbh";

    ////////////////////////////////////////////////////
    // compress a char array to a std::ostringstream
    std::ostringstream deflated;
    zlib_helper<std::ostringstream> def(deflated);
    def.init_compression()
        .compress(deflate_this, strlen(deflate_this))
        .finish();

    ////////////////////////////////////////////////////
    // use your own writer struct to write directly to a std::string
    // The advantage over std::stringstream is that it operates on the provided
    // string directly so the std::string is NOT COPIED out with .str() when needed.
    struct any_writer {
        // this struct is used as template argument below, so it SHOULD be
        // declared outside this function, but this is clearer for the example
        std::string& s_data;
        explicit any_writer(std::string& data) : s_data(data) {}
        void write(const char* d, size_t s) {
            s_data.append(d, s);
        }
    };

    std::string inflated;
    any_writer writer(inflated);
    zlib_helper<any_writer> inf(writer);
    inf.init_decompression();
    std::string deflated_temp_str = deflated.str();
    inf.decompress(deflated_temp_str.c_str(), deflated_temp_str.size());
    // no need to "finish()" since inflate knows when it is done.
    assert(inflated.c_str() == std::string(deflate_this));

    ////////////////////////////////////////////////////
    // compress a text file to a .gz file
    std::ofstream an_ofile("the_file.gz");
    zlib_helper<std::ofstream> to_gzip(an_ofile);
    to_gzip.init_compression(GZIP);
    std::ifstream an_ifile("Makefile");
    if (an_ifile.is_open()) {
        do {
            char buff[1024] = {};
            an_ifile.read(buff, 1024);
            to_gzip.add(buff, an_ifile.gcount());
        } while (!an_ifile.eof());
        // let deflate know there is no more data, so finish up.
        to_gzip.finish();
    }
    an_ifile.close();
    an_ofile.close();

    ////////////////////////////////////////////////////
    // decompress a .gz file to a text file
    deflated.clear();
    an_ofile.open("AnotherMakefile");
    zlib_helper<std::ofstream> inflator(an_ofile);
    inflator.init_decompression(GZIP);
    an_ifile.open("the_file.gz");
    if (an_ifile.is_open()) {
        do {
            char buff[512] = {};
            an_ifile.read(buff, 512);
            inflator.add(buff, an_ifile.gcount());
        } while (!an_ifile.eof());
        // again, zlib inflate knows when it is done.
    }
    an_ifile.close();
    an_ofile.close();

    assert(system("diff Makefile AnotherMakefile") == 0);
}

/OlaM
 */

#if defined __VMS
#ifndef __USE_STD_IOSTREAM
#error please "#define __USE_STD_IOSTREAM" before including this file
#endif
#include "SYS$COMMON:[libz]zlib.h"
#else

#include <zlib.h>

#endif

// build with ZLIB_HELPER_STACK_BUFFER 0 to use a vector/dynamic
// memory for the buff
// stack buffer is slightly faster but puts 16K on the stack
#if !defined ZLIB_HELPER_STACK_BUFFER
#define ZLIB_HELPER_STACK_BUFFER 1
#endif
#if !ZLIB_HELPER_STACK_BUFFER
#include <vector>
#endif
#include <cassert>

#if __cplusplus < 201103L
#define EXPLICIT
#define NOTHROW throw()
#else
#define EXPLICIT explicit
#define NOTHROW noexcept
#endif

namespace om_tools {
    namespace zlib_helper {

        // helper class to write to a std::string you supply
        class string_writer {
            std::string &s;
        public:
            explicit string_writer(std::string &ins) : s(ins) {}

            void write(const char *b, size_t size) NOTHROW {
                try {
                    s.append(b, size);
                } catch (const std::exception &e) {
                    std::cout << e.what() << '\n';
                }
            };
        };

        static const size_t COMP_CHUNK = 1024 * 16;
        enum FORMAT {
            DEFLATE,
            GZIP
        };

        // the zlib_helper
        template<class OUTPUT_TYPE>
        class zlib {

            enum DIRECTION {
                UNDECIDED,
                DEFLATING,
                INFLATING
            };
            z_stream comp_stream;
            DIRECTION direction;
            OUTPUT_TYPE &target;
            bool initialised;
#if ZLIB_HELPER_STACK_BUFFER
            unsigned char buffer[COMP_CHUNK];
#else
            std::vector<unsigned char> buffer;
#endif

            zlib &process(int fl, char *data, size_t size) NOTHROW {
                assert(initialised);
                comp_stream.avail_in = size;
                comp_stream.next_in = reinterpret_cast<unsigned char *>(data);
                do {
                    comp_stream.avail_out = COMP_CHUNK;
#if ZLIB_HELPER_STACK_BUFFER
                    comp_stream.next_out = buffer;
#else
                    comp_stream.next_out = &buffer[0];
#endif
                    int ret = direction == DEFLATING ?
                              deflate(&comp_stream, fl) : inflate(&comp_stream, fl);
                    assert(ret != Z_STREAM_ERROR);
                    if (direction == INFLATING) {
                        switch (ret) {
                            case Z_NEED_DICT:
                                ret = Z_DATA_ERROR;
                                // fall through
                            case Z_DATA_ERROR:
                            case Z_MEM_ERROR: {
                                end();
                                std::clog << "inflate error: " << ret;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    unsigned int have = COMP_CHUNK - comp_stream.avail_out;
                    if (have) {
                        target.write(reinterpret_cast<const char *>(&buffer[0]), have);
                    }
                } while (comp_stream.avail_out == 0);

                return *this;
            }

            zlib &initialise(FORMAT compression_format = DEFLATE,
                                    int compression_level = Z_DEFAULT_COMPRESSION) NOTHROW {
                int ret = Z_OK;
                if (compression_format == GZIP) {
                    if (direction == INFLATING) {
                        ret = inflateInit2(&comp_stream, 16 + MAX_WBITS);
                    } else {
                        ret = deflateInit2(&comp_stream, compression_level, Z_DEFLATED, 16 + MAX_WBITS, 8,
                                           Z_DEFAULT_STRATEGY);
                    }
                } else {
                    if (direction == DEFLATING) {
                        ret = deflateInit(&comp_stream, compression_level);
                    } else {
                        ret = inflateInit(&comp_stream);
                    }
                }
                if (ret != Z_OK) {
                    end();
                    initialised = false;
                } else {
                    initialised = true;
                }
                return *this;
            }

            void end() NOTHROW {
                if (direction == DEFLATING) {
                    deflateEnd(&comp_stream);
                } else {
                    inflateEnd(&comp_stream);
                }
                initialised = false;
            }

        public:

            // constructor, pass the container to put the result in.
            explicit zlib(OUTPUT_TYPE &t) :
                comp_stream(),
                direction(UNDECIDED),
                target(t),
                initialised(false),
#if ZLIB_HELPER_STACK_BUFFER
                buffer()
#else
            buffer(COMP_CHUNK)
#endif
            {}

            ~zlib() NOTHROW {
                if (direction != UNDECIDED) {
                    finish();
                }
                end();
            }

            EXPLICIT operator bool() const { return initialised; }

            // Initialise the object for compression
            zlib &init_compression(FORMAT compression_format = DEFLATE,
                                          int compression_level = Z_DEFAULT_COMPRESSION) NOTHROW {
                direction = DEFLATING;
                return initialise(compression_format, compression_level);
            }

            // Initialise the object for decompression
            zlib &init_decompression(FORMAT compression_format = DEFLATE,
                                            int compression_level = Z_DEFAULT_COMPRESSION) NOTHROW {
                direction = INFLATING;
                return initialise(compression_format, compression_level);
            }

            // Add data to process
            zlib &add(const char *data, size_t size) NOTHROW {
                assert(direction != UNDECIDED);
                return process(Z_NO_FLUSH, const_cast<char *>(data), size);
            }

            // Finish up compression
            // This is sufficient if all source data to compress is already available.
            zlib &compress(const char *data, size_t size) NOTHROW {
                assert(direction == DEFLATING);
                return process(Z_FINISH, const_cast<char *>(data), size);
            }

            // Finish up decompression
            // This is sufficient if all compressed data is already available.
            zlib &decompress(const char *data, size_t size) NOTHROW {
                assert(direction == INFLATING);
                return process(Z_NO_FLUSH, const_cast<char *>(data), size);
            }

            zlib &finish() NOTHROW {
                switch (direction) {
                    case DEFLATING:
                        compress("", 0);
                        break;
                    case INFLATING:
                        decompress("", 0);
                        break;
                    default:;
                }

                direction = UNDECIDED;
                return *this;
            }
        };

        // another example, IN and OUT needs to implement
        // read and write, like ifstream/ofstream.
        template<typename IN, typename OUT>
        bool compress(IN &source, OUT &target) {
            zlib<OUT> compressor(target);
            compressor.init_compression(GZIP);
            do {
                char buff[1024] = {};
                source.read(buff, 1024);
                compressor.add(buff, source.gcount());
            } while (!source.eof());
            return true;
        }
    }
}


// inplace compression (same buffer in and out)
// WIP, still only copied from Mark Adlers comment on stackoverflow, need to find some time for it
// https://stackoverflow.com/a/12412863/9767068

///* Compress buf[0..len-1] in place into buf[0..*max-1].  *max must be greater
//   than or equal to len.  Return Z_OK on success, Z_BUF_ERROR if *max is not
//   enough output space, Z_MEM_ERROR if there is not enough memory, or
//   Z_STREAM_ERROR if *strm is corrupted (e.g. if it wasn't initialized or if it
//   was inadvertently written over).  If Z_OK is returned, *max is set to the
//   actual size of the output.  If Z_BUF_ERROR is returned, then *max is
//   unchanged and buf[] is filled with *max bytes of uncompressed data (which is
//   not all of it, but as much as would fit).
//
//   Incompressible data will require more output space than len, so max should
//   be sufficiently greater than len to handle that case in order to avoid a
//   Z_BUF_ERROR. To assure that there is enough output space, max should be
//   greater than or equal to the result of deflateBound(strm, len).
//
//   strm is a deflate stream structure that has already been successfully
//   initialized by deflateInit() or deflateInit2().  That structure can be
//   reused across multiple calls to deflate_inplace().  This avoids unnecessary
//   memory allocations and deallocations from the repeated use of deflateInit()
//   and deflateEnd(). */
//int deflate_inplace(z_stream *strm, unsigned char *buf, unsigned len,
//                    unsigned *max)
//{
//    int ret;                    /* return code from deflate functions */
//    unsigned have;              /* number of bytes in temp[] */
//    unsigned char *hold;        /* allocated buffer to hold input data */
//    unsigned char temp[11];     /* must be large enough to hold zlib or gzip
//                                   header (if any) and one more byte -- 11
//                                   works for the worst case here, but if gzip
//                                   encoding is used and a deflateSetHeader()
//                                   call is inserted in this code after the
//                                   deflateReset(), then the 11 needs to be
//                                   increased to accomodate the resulting gzip
//                                   header size plus one */
//
//    /* initialize deflate stream and point to the input data */
//    ret = deflateReset(strm);
//    if (ret != Z_OK)
//        return ret;
//    strm->next_in = buf;
//    strm->avail_in = len;
//
//    /* kick start the process with a temporary output buffer -- this allows
//       deflate to consume a large chunk of input data in order to make room for
//       output data there */
//    if (*max < len)
//        *max = len;
//    strm->next_out = temp;
//    strm->avail_out = sizeof(temp) > *max ? *max : sizeof(temp);
//    ret = deflate(strm, Z_FINISH);
//    if (ret == Z_STREAM_ERROR)
//        return ret;
//
//    /* if we can, copy the temporary output data to the consumed portion of the
//       input buffer, and then continue to write up to the start of the consumed
//       input for as long as possible */
//    have = strm->next_out - temp;
//    if (have <= (strm->avail_in ? len - strm->avail_in : *max)) {
//        memcpy(buf, temp, have);
//        strm->next_out = buf + have;
//        have = 0;
//        while (ret == Z_OK) {
//            strm->avail_out = strm->avail_in ? strm->next_in - strm->next_out :
//                              (buf + *max) - strm->next_out;
//            ret = deflate(strm, Z_FINISH);
//        }
//        if (ret != Z_BUF_ERROR || strm->avail_in == 0) {
//            *max = strm->next_out - buf;
//            return ret == Z_STREAM_END ? Z_OK : ret;
//        }
//    }
//
//    /* the output caught up with the input due to insufficiently compressible
//       data -- copy the remaining input data into an allocated buffer and
//       complete the compression from there to the now empty input buffer (this
//       will only occur for long incompressible streams, more than ~20 MB for
//       the default deflate memLevel of 8, or when *max is too small and less
//       than the length of the header plus one byte) */
//    hold = strm->zalloc(strm->opaque, strm->avail_in, 1);
//    if (hold == Z_NULL)
//        return Z_MEM_ERROR;
//    memcpy(hold, strm->next_in, strm->avail_in);
//    strm->next_in = hold;
//    if (have) {
//        memcpy(buf, temp, have);
//        strm->next_out = buf + have;
//    }
//    strm->avail_out = (buf + *max) - strm->next_out;
//    ret = deflate(strm, Z_FINISH);
//    strm->zfree(strm->opaque, hold);
//    *max = strm->next_out - buf;
//    return ret == Z_OK ? Z_BUF_ERROR : (ret == Z_STREAM_END ? Z_OK : ret);
//}