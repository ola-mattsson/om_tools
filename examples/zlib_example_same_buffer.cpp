/**
 * deflate one buffer instead of an in and an out ditto
 * WIP doesn't work yet, I think
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <zlib_wrapper.h>

int main() {
    namespace ot = om_tools::zlib_helper;

    std::ifstream file("Makefile");
    std::stringstream ss; ss << file.rdbuf();
    std::string compressed_content = ss.str();
    std::string original_content = compressed_content;

    ot::same_string_writer same_writer(compressed_content);
    ot::zlib<ot::same_string_writer> comp(same_writer);
    comp.init_deflate(ot::GZIP);

    comp.deflate(&compressed_content[0], compressed_content.size());

    // truncate the string to it's hopefully smaller size
    // the same_string_writer knows how much was written.
    same_writer.truncate();

    // decompress it again so we can compare the result
    // now to an appending string_writer.
    std::string decompressed;
    ot::string_writer writer(decompressed);
    ot::zlib< ot::string_writer> decomp(writer);
    decomp.init_inflate(ot::GZIP);

    decomp.inflate(&compressed_content[0], compressed_content.size());

    // the moment of truth
    assert(original_content == decompressed);

    std::cout << "Successfully compressed to same buffer\n";
    return 0;
}
