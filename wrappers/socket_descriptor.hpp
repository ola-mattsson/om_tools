#pragma once

#include "descriptor_base.h"
#include <optional>
#include <iostream>
#include <string_view>
#include <sys/socket.h>


namespace om_tools {
namespace descriptors {
inline namespace v1_0_0 {


/**
* a socket descriptor
*/
class socket_fd : public descriptor_base<int32_t> {

public:
    socket_fd() : descriptor_base() {}
    explicit socket_fd(descriptor_type fd_) : descriptor_base(fd_) {}

    void wait_request(const socket_fd &server_socket);

    [[nodiscard]]
    socket_fd wait_request() const;

    static socket_fd create_server_socket(std::string_view port);
    static socket_fd create_client_socket(std::string_view host, std::string_view port);
};

/**
* Server socket creates a client socket
* @return the accepted client socket
*/
inline socket_fd socket_fd::wait_request() const {
    socket_fd new_sock;
    new_sock.wait_request(*this);
    return new_sock;
}

/**
* Client socket initialised from server socket.
* @param server_socket
*/
inline void socket_fd::wait_request(const socket_fd &server_socket) {
    sockaddr_in addr = {};
    socklen_t addr_len = sizeof addr;

    set(accept(server_socket.get(), reinterpret_cast<sockaddr *>(&addr), &addr_len));

    if (get() == invalid_socket) {
        perror("accept");
    }
}

class addr_info {
    using addrinfo_type = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>;

    addrinfo_type address{nullptr, freeaddrinfo};
public:

    addrinfo_type & getaddrinfo(std::string_view host, std::string_view port, std::optional<addrinfo> hints = {}) {
        if (!hints) {
            hints = addrinfo{};
            hints->ai_family = AF_INET;
            hints->ai_socktype = SOCK_STREAM;
            hints->ai_flags = host.empty() ? AI_PASSIVE : 0; // fit for bind
            hints->ai_protocol = 0;
        }
        addrinfo *result = nullptr;
        if (int s = ::getaddrinfo(host.empty() ? nullptr : host.data(), port.data(),
                                  &hints.value(), &result); s != 0) {
            std::clog << "getaddrinfo: " <<  gai_strerror(s) << '\n';
        } else {
            address.reset(result);
        }
        return address;
    }
};

/**
 * Create a server socket bound to the provided port,
 *
 * the socket_fd is size of int usually so dont worrying about return value optimization
 *
 * @param port to bind and listen to
 * @return a socket_fd
 */
inline socket_fd socket_fd::create_server_socket(std::string_view  port) {

    addr_info address;
    auto& result = address.getaddrinfo("", port.data());

    socket_fd sock(socket(result->ai_family, result->ai_socktype, result->ai_protocol));

    if (!sock.valid()) {
        perror("socket");
        return {};
    }

    int opt_val = 1;
    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
        perror("setsockopt");
        return {};
    }

    auto port_short = static_cast<int16_t>(std::strtol(port.data(), nullptr, 10));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_short);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if (bind(sock.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) == -1) {
        perror("bind");
        return {};
    }

    if (listen(sock.get(), SOMAXCONN) == -1) {
        perror("listen");
        return {};
    }

    return sock;
}

socket_fd socket_fd::create_client_socket(std::string_view host, std::string_view  port) {

    addr_info address;
    auto &result = address.getaddrinfo(host.data(), port.data());

    socket_fd client_socket;
    for (addrinfo *rp = result.get(); rp != nullptr; rp = rp->ai_next) {
        client_socket.set(socket(rp->ai_family, rp->ai_socktype,
                                 rp->ai_protocol));
        if (!client_socket.valid()) {
            continue;
        }
        if (int res = connect(client_socket.get(), rp->ai_addr, rp->ai_addrlen); res != -1) {
            break;                  /* Success */
        }
        if (rp->ai_next == nullptr) {
            std::clog << "Could not connect\n";
            return {};
        }
    }

    return client_socket;
}

}
}

using descriptors::socket_fd;
using descriptors::addr_info;

}