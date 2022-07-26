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
```c++
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
```c++
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
This does a simple select, passes a parameter and extracts the result.
scoped_result

```c++
    std::string stmt = "select i, c from a where id = $1;";
    ot::params params;
    params.add(2);
    ot::scoped_result result(default_connection().exec(stmt, params));

    if (result) {
        int32_t i = result.get_value<int32_t>(0, 2);
        std::string s = result.get_value<std::string>(0, 2);

        std::cout << s << '|' << i << '\n';
    }
```


## Descriptors
File and inet descriptors.
Wrapping a descriptor (int) has one problem, a factory function needs to pass a valid object holding the int and let the caller close it. hmm, move semantics to the rescue. 
These objects can be passed around safely and will be closed, if valid, when going out of scope. When assigned from each other and returned from functions they are moved and the original will be invalidated, so they don't accidentally close unintentionally.

Example, open a file
```c++
om_tools::file_socket open_file(std::string_view name, int32_t flags) {
    om_tools::file_socket file_d(open("a_file.txt", flags));
    if (!file_d.valid()) {
        perror("open");
        // return a new, invalid object
        return {};
    }
    else {
        // moved out since it can't be copied.
        // if the descriptor would be copied, the original
        // would be closed when going out of scope.
        return file_d;
    }
}
```