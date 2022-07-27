#pragma once

#ifdef __VMS
#ifndef __USE_STD_IOSTREAM
#define __USE_STD_IOSTREAM
#endif
#include <curl.h>
#else

#include <curl/curl.h>

#endif

#include <boost/utility/string_view.hpp>
#include <iostream>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L && !defined nullptr
#define nullptr NULL
#endif

namespace om_tools {

    namespace curl_helper {
#if __cplusplus >= 201103L
    inline namespace v1_0_0 {
#endif
        // instantiate this in main to avoid
        // repeated calls, not a huge deal but seems unnecessary
        struct curl_initialise {
            curl_initialise() {
                curl_global_init(CURL_GLOBAL_ALL);
            }

            ~curl_initialise() {
                curl_global_cleanup();
            }
        };

         __attribute__((unused))
         typedef curl_initialise curl_initialize; // oh never mind hehe it is Colour and tomaaaato, not tomaydo and Color

        /**
         * No nonsense C++ wrapper around libcurl easy api that works even in C++98\n\n
         *
         * Helps initialisation, cleanup and slightly cleaner usage code.
         * instantiate the curl_initialise struct somewhere central, such as
         * int main() or what suits your use-case.
         *
         @code
        // initialise curl somewhere central, it calls
        // curl_global_init in constructor
        // and cleanup in it's destructor
        curl_initialise init;

        // create a curl handle
        Curl_handle curl_handle; // pass (true) for verbose analytics

        // set options, like url, write function and so on
        curl_handle.set_option(CURLOPT_WRITEFUNCTION, write_data);

        // curl_slist wrapper, initialise with an array of string,
        // terminated with NULL or an empty string
        const char* headers[] = {"Content-Type: application/text", "charset: utf-8", ""};
        const curl_slist_handle slist(headers);
        curl_handle.set_option(CURLOPT_HTTPHEADER, slist.get());

        // fire it off
        curl_handle.perform();

        // see how it went
        if (curl_handle.good())
            std::cout << "Happy days\n";
        @endcode
         \n\n Or the train wreck style if you like that kind of thing, sorry Martin\n
        @code

        const char* headers[] = {"Content-Type: application/text", "charset: utf-8", ""};
        const curl_slist_handle slist(headers);
        bool success =
            Curl_handle()
                .set_option(CURLOPT_WRITEFUNCTION, write_data)
                .set_option(CURLOPT_HTTPHEADER, slist.get())
                .set_option(CURLOPT_what_ever_you_need, the values)
                .perform()
                .good();
                // the Curl_handle is cleaned up immediately at ';' since there is
                // nothing holding it.
        ...
        ...

         @endcode
         */
        class Curl_handle {
            CURL *curl;
            CURLcode result;
        public:
            explicit Curl_handle(bool verbose = false)
                : curl(curl_easy_init()), result(CURLE_OK) {
                set_option(CURLOPT_FAILONERROR, 1L);

                if (curl && verbose) {
                    set_option(CURLOPT_VERBOSE, 1L);
                }
            }

            ~Curl_handle() {
                curl_easy_cleanup(curl);
            }

            bool initialised() { return curl != nullptr; }

            template<typename OPT, typename VAL>
            Curl_handle& set_option(OPT opt, VAL val) {
                curl_easy_setopt(curl, opt, val);
                return *this;
            }

            template<typename OPT, typename VAL>
            void get_info(OPT opt, VAL val) const {
                curl_easy_getinfo(curl, opt, val);
            }

            Curl_handle& perform() {
                // get a better error message on failure
                char error_buf[CURL_ERROR_SIZE] = {};
                set_option(CURLOPT_ERRORBUFFER, error_buf);

                result = curl_easy_perform(curl);
                if (result != CURLE_OK) {
                    boost::string_view error(error_buf);
                    std::cerr << "\nlibcurl: " << result << ' ';
                    if (!error.empty()) {
                        std::cerr << error << '\n';
                    } else {
                        std::cerr << curl_easy_strerror(result) << '\n';
                    }
                }
                return *this;
            }

            bool good() const { return result == CURLE_OK; }

            CURLcode get_curl_code() const { return result; }

            struct statistics {
                curl_off_t speed_upload;
                curl_off_t total_time;
                long response_code;
            };

            statistics get_stats() const {
                statistics stats = {};
                get_info(CURLINFO_SPEED_UPLOAD_T, &stats.speed_upload);
                get_info(CURLINFO_TOTAL_TIME_T, &stats.total_time);
                get_info(CURLINFO_RESPONSE_CODE, &stats.response_code);
                return stats;
            }
        };

        class curl_slist_handle {
            struct curl_slist *slist;
        public:
            explicit curl_slist_handle(const char **strings)
                : slist(nullptr) {
                append_strings(strings);
            }

            curl_slist_handle()
                : slist(nullptr) {}

            // an array of const char* finished with null  an empty string
            void append_strings(const char **strings) {
                for (int i(0); nullptr != strings[i] && '\0' != *strings[i]; ++i) {
                    slist = curl_slist_append(slist, strings[i]);
                }
            }

            bool append_string(const char *string) {
                if (nullptr != string && *string != '\0') {
                    slist = curl_slist_append(slist, string);
                    return true;
                }
                return false;
            }

            ~curl_slist_handle() {
                curl_slist_free_all(slist);
            }

            curl_slist *get() const { return slist; }
        };
#if __cplusplus >= 201103L
    }
#endif
    }
using curl_helper::curl_initialise;
using curl_helper::curl_initialize;
using curl_helper::curl_slist_handle;
using curl_helper::Curl_handle;
}

