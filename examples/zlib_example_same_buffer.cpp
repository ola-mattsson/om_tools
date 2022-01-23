/**
 * compress one buffer instead of an in and an out ditto
 * WIP
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <zlib_wrapper.h>

int main() {
    namespace ot = om_tools::zlib_helper;

    std::ifstream file("Makefile");
    std::stringstream ss; ss << file.rdbuf();
    std::string data = ss.str();
    ot::string_writer writer(data);

    ot::zlib<ot::string_writer> comp(writer);
    comp.compress(data.c_str(), data.size());

    std::cout << "done\n";
}
