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

#define VERBOSE 0

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

class CURL_handle {
    CURL* curl;
    CURLcode result;
public:
    explicit CURL_handle(bool verbose = false)
            :curl(curl_easy_init()), result(CURLE_OK) {
        set_option(CURLOPT_FAILONERROR, 1L);

        if (curl && verbose) {
            set_option(CURLOPT_VERBOSE, 1L);
        }
    }

    ~CURL_handle() {
        curl_easy_cleanup(curl);
    }

    bool initialised() { return curl != NULL; }

    template<typename OPT, typename VAL>
    void set_option(OPT opt, VAL val) {
        curl_easy_setopt(curl, opt, val);
    }
    template<typename OPT, typename VAL>
    void get_info(OPT opt, VAL val) const {
        curl_easy_getinfo(curl, opt, val);
    }
    CURLcode perform() {
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
        return result;
    }

    bool get_curl_ok() const { return result == CURLE_OK; }

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
    struct curl_slist* slist;
public:
    explicit curl_slist_handle(const char* strings[])
            :slist(NULL) {
        append_strings(strings);
    }

    curl_slist_handle()
            :slist(NULL) { }

    void append_strings(const char* strings[]) {
        for (int i(0); NULL != strings[i] && '\0' != *strings[i]; ++i) {
            slist = curl_slist_append(slist, strings[i]);
        }
    }

    ~curl_slist_handle() {
        curl_slist_free_all(slist);
    }

    curl_slist* get() const { return slist; }
};
