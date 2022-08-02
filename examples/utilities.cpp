//
// Created by Ola Mattsson on 2022-08-08.
//
#include <iostream>

#define DB_BOOL_t_f
#include <algorithm.h>

int main() {
    std::cout
        << om_tools::lex_cast<int>("10") << '\n'
        << om_tools::lex_cast<double>("1.9") << '\n'
        << std::boolalpha << om_tools::lex_cast<bool>("true") << '\n'
            ;
}

