# this is my toolbox

Small, more or less self-contained C++ wrappers of some C apis and types.
Main reason is cleanup and complexity, NOT OOP modelling, go somewhere else for that stuff.

This is work in progress, donno if it will ever be in a _finished_ state. Might still be useful to somebody


Some intentions:
1. Header only
2. Simple/straight forward to start using.
3. Tests are examples and documentation

This project has a bunch of dependencies, some are for what they wrap, like libz, others are for tests and examples.

Examples build with cmake. 

There is a conanfile.py, but you don't have to use it. Some dependencies have Find modules provided by CMake, some don't, but I'm not providing those that don't. If you use conan, they will be created automatically. This way they don't pollute your file system, just to test these examples.


## Curl

Uses all the well known options and structures declared by curl but in a slightly cleaner way. 
This example is compact but "hides" nothing other than the actual freeing of the curl object, the C++ way.
```
const char* headers[] = {"Content-Type: application/text", "charset: utf-8", ""};
const curl_slist_handle slist(headers);
bool success =
    Curl_handle()
        .set_option(CURLOPT_WRITEFUNCTION, write_data)
        .set_option(CURLOPT_HTTPHEADER, slist.get())
        .set_option(CURLOPT_what_ever_you_need, the values)
        .perform()
        .good();
        // the Curl_handle is cleaned up immediately at ';' since the object is not owned.
```

## zlib

Deflates and inflates data to anything that responds to `void write(const char *b, size_t size)` such as std::ofstream. A string_writer is provided that compresses to a std::string you provide.
```
void compress(const char *from, const char *to) {
    namespace ot = om_tools::zlib_helper;
    std::ifstream from_file(from);
    std::ofstream to_file(to);
    if (from_file.is_open() && to_file.is_open()) {
        ot::zlib <std::ofstream> compressor(to_file);
        compressor.init_compression(ot::GZIP);
        do {
            char buff[1024] = {};
            from_file.read(buff, 1024);
            compressor.add(buff, from_file.gcount());
        } while (!from_file.eof());
    }
}

```

## libpq

All libpg C++ interfaces seems to be C++11 or higher, I needed one for 98.