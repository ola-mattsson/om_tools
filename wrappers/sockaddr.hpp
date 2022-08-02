#pragma once

#include <tuple>
#include <variant>
#include <string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <algorithm.h>

namespace om_tools {
#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif

class Sockaddr {
    std::variant<sockaddr_un, sockaddr_in> m_variants;

public:
    Sockaddr(std::string_view target, std::string_view port) {
        if (port.empty()) {
            m_variants.emplace<sockaddr_un>();

            auto &un = std::get<sockaddr_un>(m_variants);
            un.sun_family = AF_UNIX;
//            un.sun_len =
            utilities::copy_array(un.sun_path, target);
        } else {
            m_variants.emplace<sockaddr_in>();

            auto port_short = static_cast<int16_t>(std::strtol(port.data(), nullptr, 10));

            auto &in = std::get<sockaddr_in>(m_variants);
            in.sin_family = AF_INET;
            in.sin_port = htons(port_short);
            in.sin_addr.s_addr = htonl(INADDR_ANY);
        }
    }

    std::tuple<sockaddr *, socklen_t> get_sockaddr() {
        if (std::holds_alternative<sockaddr_un>(m_variants)) {
            auto &variant = std::get<sockaddr_un>(m_variants);
            unlink(variant.sun_path);
            return {reinterpret_cast<sockaddr *>(&variant), sizeof variant};
        } else if (std::holds_alternative<sockaddr_in>(m_variants)) {
            auto &variant = std::get<sockaddr_in>(m_variants);
            return {reinterpret_cast<sockaddr *>(&variant), sizeof variant};
        }
        return {};
    }
};

#if __cplusplus >= 201103L
}
#endif

}
