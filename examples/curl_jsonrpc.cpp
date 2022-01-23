#include <curl_wrapper.h>
#include <sstream>
#include <curl/curl.h>
#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L && !defined nullptr
#define nullptr NULL
#endif

std::string make_jsonrpc() {
    std::stringstream ss;
    namespace uuid = boost::uuids;
    uuid::random_generator random_ids;

    ss << "[";
    size_t max = 5;
    for (size_t i = 0; i != max; ++i) {
        ss << "{";
        ss  << "  \"id\": \"" << random_ids() << "\",\n"
            << "  \"params\": {\n"
            << "    \"some_value\": \"" <<random_ids() << "\",\n"
            << "    \"some_other_value\": 1\n"
            << "  },\n"
            << "  \"method\": \"someMethod\",\n"
            << "  \"jsonrpc\": \"2.0\"\n";
        ss << "}";
        if (i+1 == max) {
            ss << "\n";
        } else {
            ss << ",\n";
        }
    }

    ss << "]";
    return ss.str();
}

int test_send() {
    namespace otc = om_tools::curl_helper;

    std::string json = make_jsonrpc();

    /* init the curl session */
    otc::Curl_handle curl_handle;

    /* some headers, just to show the curl_list_handle utility*/
    const char* headers[] = {"Content-Type: application/text",
                             "charset: utf-8",
                             ""};
    const otc::curl_slist_handle slist(headers);
    curl_handle.set_option(CURLOPT_HTTPHEADER, slist.get())
                .set_option(CURLOPT_URL, "http://somehost")
                .set_option(CURLOPT_POSTFIELDS, json.c_str())
                .set_option(CURLOPT_POSTFIELDSIZE, json.size());

    if (!curl_handle.perform().good()) {
        std::cout << "Failed\n";
    }

    return 0;
}

int main() {
    om_tools::curl_helper::curl_initialise init;

    for (int i = 0; i < 1000; ++i) {
        for (int j = 0; j < 2; ++j) {
            test_send();
        }
        sleep(1);
    }

    return 0;
}

