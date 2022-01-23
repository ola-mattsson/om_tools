#include <curl_wrapper.h>
#include <fstream>
#include <curl/curl.h>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L && !defined nullptr
#define nullptr NULL
#endif

/*
 * rewrite of https://curl.se/libcurl/c/url2file.html
 * This wrapper is really simple but cleans up the noice a bit, fewer words and automatic cleanup
 */

// the write function passed in CURLOPT_WRITEDATA,
// note how C, which curl is written in doesn't care if the
// "ptr" and "stream" are void* or their actual type, here "const char*" and
// "std::ofstream" which is a C++ class. saves reinterpret_casting them/less code.
// Also, size and nmemb are size_t (unsigned) in libcurl, but libcurl is not likely to pass
// std::numeric_limits<long>::max() in one call. unnecessary compiler warning avoided
static size_t write_data(const char *ptr, ssize_t size, ssize_t nmemb, std::ofstream *out_file) {
    out_file->write(ptr, size * nmemb);
    return size * nmemb;
}

int if_you_like_stacked_oneliners();
int much_like_the_original_example();

int main() {
    om_tools::curl_helper::curl_initialise init;
    if_you_like_stacked_oneliners();
    much_like_the_original_example();
}

int much_like_the_original_example() {
    namespace otc = om_tools::curl_helper;

    /* init the curl session */
    otc::Curl_handle curl_handle;

    std::string url = "https://developer.mozilla.org/en-US/docs/"
                      "Web/HTTP/Basics_of_HTTP/MIME_types/Common_types";

    /* set URL to get here */
    curl_handle.set_option(CURLOPT_URL, url.c_str());

    /* disable progress meter, set to 0L to enable it */
    curl_handle.set_option(CURLOPT_NOPROGRESS, 1L);

    /* send all data to this function  */
    curl_handle.set_option(CURLOPT_WRITEFUNCTION, write_data);

    /* some headers, just to show the curl_list_handle utility*/
    const char *headers[] = {"Content-Type: application/text", "charset: utf-8", ""};
    const otc::curl_slist_handle slist(headers);
    curl_handle.set_option(CURLOPT_HTTPHEADER, slist.get());

    /* open the file */
    std::ofstream out_file("page.html");
    if (out_file) {
        /* write the page body to this file  */
        curl_handle.set_option(CURLOPT_WRITEDATA, &out_file);

        /* get it! */
        if (curl_handle.perform().good()) {
            std::cout << "Success\n";
        } else {
            std::clog << "Failed\n";
        }
    }

    return 0;
}

int if_you_like_stacked_oneliners() {
    namespace otc = om_tools::curl_helper;

    std::ofstream out_file("page1.html");

    if (!out_file.is_open()) {
        std::clog << "Cannot open out file.\n";
        return 1;
    }
    const char *headers[] = {"Content-Type: application/text", "charset: utf-8", ""};
    const otc::curl_slist_handle slist(headers);

    if (otc::Curl_handle()
        .set_option(CURLOPT_HTTPHEADER, slist.get())
        .set_option(CURLOPT_URL, "https://developer.mozilla.org/en-US/docs/"
                                 "Web/HTTP/Basics_of_HTTP/MIME_types/Common_types")
        .set_option(CURLOPT_NOPROGRESS, 1L)
        .set_option(CURLOPT_WRITEFUNCTION, write_data)
        .set_option(CURLOPT_WRITEDATA, &out_file)
        // go go go go
        .perform()
        .good()) {
        std::cout << "Success\n";
    } else {
        std::clog << "Failed\n";
    }

    return 0;

}
