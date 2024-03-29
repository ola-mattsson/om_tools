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

#include <cstring>
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
#if __cplusplus >= 201103L
    inline namespace  v1_0_0 {
#endif

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
                              ::deflate(&comp_stream, fl) : ::inflate(&comp_stream, fl);
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
            zlib &init_deflate(FORMAT compression_format = GZIP,
                               int compression_level = Z_DEFAULT_COMPRESSION) NOTHROW {
                direction = DEFLATING;
                return initialise(compression_format, compression_level);
            }

            // Initialise the object for decompression
            zlib &init_inflate(FORMAT compression_format = GZIP,
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
            zlib &deflate(const char *data, size_t size) NOTHROW {
                assert(direction == DEFLATING);
                return process(Z_FINISH, const_cast<char *>(data), size);
            }

            // Finish up decompression
            // This is sufficient if all compressed data is already available.
            zlib &inflate(const char *data, size_t size) NOTHROW {
                assert(direction == INFLATING);
                return process(Z_NO_FLUSH, const_cast<char *>(data), size);
            }

            zlib &finish() NOTHROW {
                switch (direction) {
                    case DEFLATING:
                        deflate("", 0);
                        break;
                    case INFLATING:
                        inflate("", 0);
                        break;
                    default:;
                }

                direction = UNDECIDED;
                return *this;
            }
        };

        // convenience constructors
        template<class OUT>
        struct zlib_deflator : public zlib<OUT> {
            explicit zlib_deflator(OUT& out, FORMAT compression_format = GZIP,
                          int compression_level = Z_DEFAULT_COMPRESSION) : zlib<OUT>(out) {
                this->init_deflate(compression_format, compression_level);
            }
        };

        template<class OUT>
        struct zlib_inflator : public zlib<OUT>  {
            explicit zlib_inflator(OUT& out, FORMAT compression_format = GZIP,
                          int compression_level = Z_DEFAULT_COMPRESSION) : zlib<OUT>(out) {
                this->init_inflate(compression_format, compression_level);
            }
        };

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

        // Helper class that allows for a std::string to be compressed
        // to itself.
        // WIP, needs a lot of testing. If data doesn't compress very
        // well the compressed section might overlap the not yet
        // processed section.
        // https://stackoverflow.com/a/12412863
        class same_string_writer {
            std::string &str;
            std::string::size_type offset;
        public:
            explicit same_string_writer(std::string &ins) : str(ins), offset(0) {}

            void write(const char *b, size_t size) NOTHROW {
                try {
                    // copy into space already processed
                    memcpy(&str[offset], b, size);
                    offset += size;
                } catch (const std::exception &e) {
                    std::cout << e.what() << '\n';
                }
            };

            // truncate the string to it's compressed size.
            void truncate() {
                str.resize(offset);
            }
            static bool maybe_gzip_deflated(std::istream& data_stream);
            static bool maybe_gzip_deflated(const char* three_bytes);

        };


        // another example, IN and OUT needs to implement
        // read and write, like ifstream/ofstream.
        template<typename IN, typename OUT>
        bool compress(IN &source, OUT &target) {
            zlib<OUT> compressor(target);
            compressor.init_deflate(GZIP);
            do {
                char buff[1024] = {};
                source.read(buff, 1024);
                compressor.add(buff, source.gcount());
            } while (!source.eof());
            return true;
        }
#if __cplusplus >= 201103L
    }
#endif
        /**
         * See if the buffer starts with 3 particular bytes. This is not a full proof
         * test but those bytes are rather unusual in text data so the false negative
         * should be a rare occurrence.
         * The 3:rd byte is 08 if the gzip is DEFLATED so not strictly the gzip header
         * but this is what `tar zcf` does for example.
         * The purpose of this test is to check if data may be risky to compress into
         * it self, there are a lot of compression formats that may make inplace
         * compression less safe.
         *
         * So, if you know your data, go ahead compress in place but be careful.
         *
         * @param a std::istream
         * @return
         */
        bool same_string_writer::maybe_gzip_deflated(const char* three_bytes) {
            static const char gzip_header[] = {'\x1f', '\x8b', '\x08'};

            return three_bytes[0] == gzip_header[0] &&
                   three_bytes[1] == gzip_header[1] &&
                   three_bytes[2] == gzip_header[2];
        }

        bool same_string_writer::maybe_gzip_deflated(std::istream& data_stream) {
            char three_bytes[3] = {};
            data_stream.read(three_bytes, 3);
            data_stream.seekg(0, std::ios::beg);

            return maybe_gzip_deflated(three_bytes);
        }

    }
// export names to namespace om_tools
using zlib_helper::string_writer;
using zlib_helper::zlib;
using zlib_helper::zlib_deflator;
using zlib_helper::zlib_inflator;
using zlib_helper::same_string_writer;
using zlib_helper::GZIP;
using zlib_helper::DEFLATE;
}
