#include <curl_wrapper.h>

#include <curl/curl.h>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L && !defined nullptr
#define nullptr NULL
#endif

/*
 * rewrite of https://curl.se/libcurl/c/url2file.html
 * This wrapper is really simple but cleans up the noice a bit, fewer words and automatic cleanup
 */

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

int main() {
    curl_initialise init;
    static const char *page_filename = "page.html";
    FILE *page_file = nullptr;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    CURL_handle curl_handle(true);

    /* set URL to get here */
    curl_handle.set_option(CURLOPT_URL, "https://appola.se/index.html");

    /* disable progress meter, set to 0L to enable it */
    curl_handle.set_option(CURLOPT_NOPROGRESS, 1L);

    /* send all data to this function  */
    curl_handle.set_option(CURLOPT_WRITEFUNCTION, write_data);

    /* open the file */
    page_file = fopen(page_filename, "wb");
    if (page_file) {

        /* write the page body to this file handle */
        curl_handle.set_option(CURLOPT_WRITEDATA, page_file);

        /* get it! */
        curl_handle.perform();

        /* close the header file */
        fclose(page_file);
    }

    return 0;
}

