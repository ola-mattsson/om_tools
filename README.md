# this is my toolbox

C++, generic do everything, swiss army knife, one-stop shop, C++ wrappers blah blah blah... smells bloated to me. I'm no fan of generic C++ wrappers, so I thought I should write some :P 

Use them as examples, copy if you like and tweak to your exact needs, the C++ way.

Premises:
1. Header only
2. Simple and light-weight. 
3. Independent, you can just take the headers you need, no need to depend on this
4. Tests are examples and documentation
5. C++ standard, hmm waiting for the jury about this. Some are "stuck" in C++98, some things can be written to take advantage of newer C++ when available at no extra cost to C++98 code.

Lets if the premises hold.

This project has a bunch of dependencies, some are for what they wrap, like libz, others are for tests and examples.

A lot of these wrappers were written to actually learn the C api they wrap.

Examples build with cmake.

I use conan, but you don't have to. Some dependencies have Find modules provided by CMake, some don't, but I'm not providing those that don't. If you use conan, they will be created automatically. This way they don't pollute your file system, just to test these examples.

### some WIP links 
https://www.tutorialspoint.com/postgresql/postgresql_c_cpp.htm

