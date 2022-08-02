//
// Created by Ola Mattsson on 2022-08-08.
//
#pragma once
#include <string_view>
#include <sstream>

#if defined BOOST_VERSION
#include <boost/lexical_cast.hpp>
#endif

namespace om_tools {
namespace utilities {
#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif

/**
 * If boost is available, it's lexical_cast is very fast and capable., but if converting a string is all
 * else this does the trick but
 * is no where near as fast, BUT, it's safe! Throws std::bad_cast when necessary
 *
 * ".7" successfully converts to double 0.69999... (lots of nines)
 *      and throws std::bad_cast if int is requested
 * "0.7" successfully converts to int 0 and double 0.6999... (more nines)
 * "Ten" throws std::bad_cast if any numeric type is requested
 *
 * Usage:
 *  int i = lex_cast<int>("10");
 *
 * @tparam T expected type
 * @param row
 * @param col
 * @return converted item
 */

template<typename T>
static T lex_cast(std::string_view source) {

#if defined DB_BOOL_t_f
    // boolean string from at least Postgres is 't'/'f', get rid of this if you dont need it
    if (std::is_same<T, bool>::value) {
        switch (source[0]) {
            case 't':
            case 'T':
            case '1':
                return true;
            case 'f':
            case 'F':
            case '0':
                return false;
        }
    }
#endif

#if defined BOOST_VERSION
    return boost::lexical_cast<T>(string_value);
#else
#if __cplusplus >= 201103L
    T out{};
#else
    T out = T();
#endif
    std::stringstream ss;
    ss << source;
    ss >> out;
    if (ss.fail() && std::is_arithmetic<T>::value) {
        throw std::bad_cast();
    }
    return out;
#endif
}

// safely copying the string_view to a char ARRAY, the point being that this will
// not compile with a char pointer and the compiler KNOWS the destination capacity
template <size_t LEN>
size_t copy_array(char (&dst)[LEN], std::string_view src) {
    dst[src.copy(dst, LEN)] = '\0';
    return std::min(LEN, src.length());
}


#if __cplusplus >= 201103L
}
#endif
}
using utilities::lex_cast;
}
