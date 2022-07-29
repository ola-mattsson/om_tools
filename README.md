# this is my toolbox

Small, more or less self-contained C++ wrappers of some C apis and types.
Main reason is cleanup and complexity, not OOP modelling.

This is work in progress, donno if it will ever be in a _finished_ state. Might still be useful to somebody


Some intentions:
1. Header only
2. Simple/straight forward to start using.
3. Use fancy C++ stuff things when they help, makes sense and improve readability. This is not a C++ pageant.
4. Tests are examples and documentation

C++98 is still a thing in places, sadly one might say byt that's just opinion, you can do nice things in 98 too, 
Some of the wrappers require higher C++ std newer than 98, particularly the descriptors since the core solution is using move semantics. So once in I used optional and string_view, unique_ptr and some language features earlier requiring boost.
The descriptors can probably be done with C++98 as well but I choose not to. `#if`'ing it in would just be messy and horrible, I want the wrappers to be readable and maintainable.

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
        // the CURL object managed by Curl_handle is cleaned up immediately at ';' since the object is not owned.
        // the curl_slist managed by curl_slist_handle is freed at the end of it's scope.
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
        compressor.init_deflate(ot::GZIP);
        do {
            char buff[1024] = {};
            from_file.read(buff, 1024);
            compressor.add(buff, from_file.gcount());
        } while (!from_file.eof());
    }
}

```

Same buffer compression, working on it.



## PostgreSQL - libpq

All libpg C++ libraries seems to require C++11 or higher, I needed one for 98.
This example does a simple select, passes a parameter and extracts the result.
`scoped_result` will free the PGresult data when going out of scope or is given new data.

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
File and socket descriptors.

Wrapping a descriptor (int) has one problem, for example a factory function like the below, needs to return a valid object holding the int and let the caller close it when ITs scope finishes, i.e. not closing when the factory function ends. hmm, move semantics to the rescue. 

These objects can be passed around safely and will be closed, if valid, when going out of scope. When assigned from each other and returned from functions they are moved and the original will be invalidated, so they don't accidentally close.

Why not use copy constructor/copy assignment to do the same thing? One could, but this would be too weird, assigning one with another invalidates the original, this would surprise the user which is bad. There are probably ways to do that too but this works as intended.

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
        // if the descriptor were copied, the original
        // would be closed when going out of scope.
        return file_d;
    }
}
```

The inet socket example is a bit big for this readme but the essentials concerning descriptors are the same as the file example.

Have fun and let me know if something is bad, odd or whatever questions you may have.

### Todo
Use a test framework for the tests. This will surely inspire some redesign, since I didn't to this first.
