/**
 * Some examples using zlib_helper
 *
 * baseline
 * 1. you provide the input and out put
 * 2. you are responsible for making sure they are ready to read / write
 *
 * The example blocks are surrounded with { and } to show how scope matters,
 * in case you are new to C++. Those blocks could be functions,
 * however, the last example shows how this optional
 */

#include <iostream>
#include <fstream>
#include <zlib_helper.h>

int main() {
    namespace ou = olas_utils;
    static const size_t K_BYTE = 1024;
    static const size_t BUFF_SIZE = K_BYTE;

    {
        std::ifstream read_this("Makefile");
        if (read_this.is_open()) {
            std::ofstream compressed("Makefile.gz");

            ou::zlib_helper<std::ofstream> deflator(compressed);
            deflator.init_compression(ou::GZIP);

            do {
                char buff[BUFF_SIZE] = {};
                read_this.read(buff, BUFF_SIZE);
                deflator.add(buff, read_this.gcount());
            } while (!read_this.eof());

            // it is possible to call deflator.finish() here but
            // zlib_helper destructor will call it if it wasn't called explicitly
            // So. if you want to close the target (the compressed object above),
            // make sure  to call zlib_helper.finish() before or the file will
            // be incomplete, also the deflator will try to write to it.

            // The target object must exist and be passed to the zlib_helpers
            // constructor. This ensures that the target is destructed/closed AFTER
            // the zlib_helpers destructor has finished/flushed.
            // i.e. it is safe to just let the target and the zlib_helper
            // auto destruct.

            std::cout << "done compressing the file, well actually will be after the {\n";
        }
    }

    // decompress to a std::string, use the string_writer class or write your own
    // This demonstrates that zlib_helper only calls a write function of any object.
    std::string str;
    {
        std::ifstream read_this("Makefile.gz");
        if (read_this.is_open()) {
            ou::string_writer to_this(str);
            ou::zlib_helper<ou::string_writer> inflator(to_this);
            inflator.init_decompression(ou::GZIP);
            do {
                char buff[BUFF_SIZE] = {};
                read_this.read(buff, BUFF_SIZE);
                inflator.add(buff, read_this.gcount());
            } while (!read_this.eof());
        }
    }
    // dump it on a file, simple. this is not optimal if BIGGER data amounts
    std::ofstream decompressed("Makefile.copy");
    decompressed << str;
    decompressed.close();
    assert(system("diff Makefile Makefile.copy") == 0);


    // decompress to a std::ofstream, same-same really
    // this also shows the above "scope blocked" examples is
    // not enforced, it's only easier/less code.
    // the compressor is finished and the file is closed manually below
    std::ifstream read_from_this("Makefile.gz");
    std::ofstream write_to_this("Makefile_decomp");
    ou::zlib_helper<std::ofstream> decomp(write_to_this);
    decomp.init_decompression(ou::GZIP);
    do {
        char buff[BUFF_SIZE] = {};
        read_from_this.read(buff, BUFF_SIZE);
        decomp.add(buff, read_from_this.gcount());
    } while (!read_from_this.eof());

    // manually finish and close
    decomp.finish();
    write_to_this.close();

    assert(system("diff Makefile Makefile_decomp") == 0);


    // how simple it can get
    std::ifstream in_thing("Makefile");
    std::ofstream out_thing("Makefile_1");
    if (in_thing.is_open() && out_thing.is_open()) {
        ou::compress(in_thing, out_thing);
        in_thing.close();
        out_thing.close();
    }



    std::cout << "done\n";
}
