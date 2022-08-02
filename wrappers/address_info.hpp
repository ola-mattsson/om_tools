#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <netdb.h>

namespace om_tools {
#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif
// TODO this is not right, for TCP (AF_INET) the addrinfo is malloced by getaddrinfo but for UDS it's newed... fixit
    // a bit surprised asan doesn't fail this

class Addr_info {
    using addrinfo_type = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>;

    addrinfo_type address{nullptr, freeaddrinfo};
public:

    /**
     * Initialise an addrinfo object. host is hostname for TCP or filename for UDS.
     * If port is empty string, UDS is assumed.
     * @param host TCP host name or UDS filename
     * @param port port or empty for UDS
     * @param hints an optionally provided addrinfo object
     * @return
     */
    addrinfo_type &
    getaddrinfo(std::string_view host, std::string_view port, std::optional<addrinfo> hints = {}) {
        if (!hints) {
            hints = addrinfo{};
            hints->ai_family = port.empty() ? AF_UNIX : AF_INET;
            hints->ai_socktype = SOCK_STREAM;
//            hints->ai_flags = port.empty() ? 0 : AI_PASSIVE; // fit for bind
            hints->ai_flags = host.empty() ? AI_PASSIVE : 0; // fit for bind
            hints->ai_protocol = 0;
        }
        addrinfo *result = nullptr;
        if (hints->ai_family == AF_INET) {
            if (int s = ::getaddrinfo(host.empty() ? nullptr : host.data(), port.data(),
                                      &hints.value(), &result); s != 0) {
                std::clog << "getaddrinfo: " << gai_strerror(s) << '\n';
            } else {
                address.reset(result);
            }
        } else {
            address.reset(new addrinfo(hints.value()));
        }
        return address;
    }
};
}

#if __cplusplus >= 201103L
}
#endif

