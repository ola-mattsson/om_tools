#pragma once

#include "sockaddr.hpp"
#include "address_info.hpp"
#include "descriptor_base.hpp"
#include <optional>
#include <iostream>
#include <memory>
#include <string_view>
#include <sys/socket.h>
#include <cstring>


namespace om_tools {
namespace descriptors {

#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif

/**
* a socket descriptor
*/
class Socket : public Descriptor_base<int32_t> {

public:
    Socket() : Descriptor_base() {}
    explicit Socket(descriptor_type fd) : Descriptor_base(fd) {}

    void wait_request(const Socket &server_socket);

    [[nodiscard]]
    Socket wait_request() const;

    static Socket create_server_socket(std::string_view name, std::string_view port);
//    static IP_socket create_client_socket(std::string_view host, std::string_view port);

    static Socket create_tcp_server_socket(std::string_view port);
    static Socket create_tcp_client_socket(std::string_view host, std::string_view port);

    static Socket create_uds_server_socket(std::string_view name);
    static Socket create_uds_client_socket(std::string_view name);
};

/**
* Server socket creates a client socket
* @return the accepted client socket
*/
inline Socket Socket::wait_request() const {
    Socket new_sock;
    new_sock.wait_request(*this);
    return new_sock;
}

/**
* Client socket initialised from server socket.
* @param server_socket
*/
inline void Socket::wait_request(const Socket &server_socket) {
    sockaddr_in addr = {};
    socklen_t addr_len = sizeof addr;

    set(accept(server_socket.get(), reinterpret_cast<sockaddr *>(&addr), &addr_len));

    if (get() == invalid_socket) {
        perror("accept");
    }
}

//std::variant<sockaddr_un, sockaddr_in> get_sockaddr(std::string_view target, int16_t port) {
//    if (port > 0) {
//        sockaddr_in in;
//        in.sin_family = AF_INET;
//        in.sin_port = htons(port);
//        in.sin_addr.s_addr = htonl(INADDR_ANY);
//        return in;
//    } else {
//        sockaddr_un un;
//        un.sun_family = AF_UNIX;
//        target.copy(un.sun_path, target.length());
//        un.sun_len = target.length() +1;
//        return un;
//    }
//}

/**
 * Create a server socket bound to the provided port,
 *
 * the socket_fd is size of int usually so dont worrying about return value optimization
 *
 * @param port to bind and listen to
 * @return a socket_fd
 */
inline Socket Socket::create_tcp_server_socket(std::string_view  port) {
    return create_server_socket("", port);
//    Addr_info address;
//    auto &address_info = address.getaddrinfo("", port.data());
//
//    auto sock = IP_socket(socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol));
//
//    if (!sock.valid()) {
//        std::clog << __FUNCTION__ << ": socket failed: " << strerror(errno);
//        return {};
//    }
//
//    auto opt_val = 1;
//    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
//        std::clog << __FUNCTION__ << ": setsockopt failed: " << strerror(errno);
//        return {};
//    }
//
//    auto port_short = static_cast<int16_t>(std::strtol(port.data(), nullptr, 10));
//    auto addr = std::get<sockaddr_in>(get_sockaddr("", port_short));
//
//    if (bind(sock.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) == -1) {
//        std::clog << __FUNCTION__ << ": bind failed: " << strerror(errno);
//        return {};
//    }
//
//    if (listen(sock.get(), SOMAXCONN) == -1) {
//        std::clog << __FUNCTION__ << ": listen failed: " << strerror(errno);
//        return {};
//    }
//
//    return sock;
}

inline Socket Socket::create_uds_server_socket(std::string_view name) {
    return create_server_socket(name, "");
//    Addr_info address;
//    auto &address_info = address.getaddrinfo(name, "");
//
//    auto sock = IP_socket(socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol));
//
//    if (!sock.valid()) {
//        std::clog << __FUNCTION__ << ": socket failed: " << strerror(errno);
//        return {};
//    }
//
//    auto opt_val = 1;
//    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
//        std::clog << __FUNCTION__ << ": setsockopt failed: " << strerror(errno);
//        return {};
//    }
//
//    auto addr = std::get<sockaddr_un>(get_sockaddr(name, -1));
//
//    unlink(name.data());
//    if (bind(sock.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) == -1) {
//        std::clog << __FUNCTION__ << ": bind failed: " << strerror(errno);
//        return {};
//    }
//
//    if (listen(sock.get(), SOMAXCONN) == -1) {
//        std::clog << __FUNCTION__ << ": listen failed: " << strerror(errno);
//        return {};
//    }
//
//    return sock;
}

/**
 * Create a server socket bound to the provided port for TCP, or filename for UDS
 *
 * the socket_fd is size of int usually so dont worrying about return value optimization
 *
 * @param port to bind and listen to
 * @return a socket_fd
 */
inline Socket Socket::create_server_socket(std::string_view name, std::string_view port) {

    Addr_info address;
    // if port is empty, we want udp, i.e. name is a filename
    auto &address_info = address.getaddrinfo(name, port);
    auto sock = Socket(socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol));

    if (!sock.valid()) {
        std::clog << __FUNCTION__ << ": socket failed: " << strerror(errno);
        return {};
    }

    auto opt_val = 1;
    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
        std::clog << __FUNCTION__ << ": setsockopt failed: " << strerror(errno);
        return {};
    }

    Sockaddr sockaddr(name, port);
    const auto [addr, sock_len] = sockaddr.get_sockaddr();
    if (bind(sock.get(), addr, sock_len) == -1) {
        std::clog << __FUNCTION__ << ": bind failed: " << strerror(errno);
        return {};
    }

    if (listen(sock.get(), SOMAXCONN) == -1) {
        std::clog << __FUNCTION__ << ": listen failed: " << strerror(errno);
        return {};
    }

    return sock;
}

Socket Socket::create_tcp_client_socket(std::string_view host, std::string_view  port) {

    Addr_info address;
    auto &result = address.getaddrinfo(host.data(), port.data());

    Socket client_socket;
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

Socket Socket::create_uds_client_socket(std::string_view name) {

    Socket client(socket(AF_UNIX, SOCK_STREAM, 0));
    struct sockaddr_un     server_address{};
    name.copy(server_address.sun_path, name.size());
    int32_t connect_rc = connect(client.get(), reinterpret_cast<struct sockaddr*> (&server_address), sizeof server_address);
    if (connect_rc == -1) {
        std::clog << __FUNCTION__ << " connect failed: " << strerror(errno);
    }
    return client;
}

#if __cplusplus >= 201103L
}
#endif

}
using descriptors::Socket;
}
