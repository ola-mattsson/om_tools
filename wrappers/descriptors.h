#pragma once

/**
 * a socket wrapper, of sorts
 *
 * An instance of socket_fd WILL close when it goes out of scope
 *
 * about the inline namespace
 * https://youtu.be/rUESOjhvLw0
 */

#include <optional>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>

namespace om_tools {
namespace descriptors {
inline namespace v1_0_0 {

struct descriptor_base {
    using descriptor_type = int32_t;
    static const descriptor_type invalid_socket = -1;
private:
    descriptor_type fd{invalid_socket};
public:
    descriptor_base() = default;

    explicit descriptor_base(descriptor_type fd_) : fd(fd_) {}

    // cannot be copied
    explicit descriptor_base(const descriptor_base &) = delete;

    // can be moved, the source will be invalidated
    descriptor_base(descriptor_base &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
    }

    // close if valid
    ~descriptor_base() { if (valid()) { close(); }}

    // cannot be copy assigned
    descriptor_base &operator=(const descriptor_base &other) = delete;

    // can be move-assigned, the source will be invalidated
    descriptor_base &operator=(descriptor_base &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    void set(descriptor_type desc) {
        fd = desc;
    }
    [[nodiscard]]
    descriptor_type get() const {
        return fd;
    }

    void close() const {
        ::close(fd);
    }

    [[nodiscard]]
    bool valid() const { return fd != invalid_socket; }

    explicit operator descriptor_type() const { return fd; }

};

// no need to declare a class for file sockets, this is done so just provide an alias
using file_descriptor = descriptor_base;

class socket_fd : public descriptor_base {

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
    socklen_t addrlen = sizeof addr;

    set(accept(server_socket.get(), reinterpret_cast<sockaddr *>(&addr), &addrlen));

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
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }
        address.reset(result);
        return address;
    }
};

inline socket_fd socket_fd::create_server_socket(std::string_view  port) {

    addr_info address;
    auto& result = address.getaddrinfo("", port.data());

    socket_fd sock(socket(result->ai_family, result->ai_socktype, result->ai_protocol));
    int opt_val = 1;
    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
        std::cout << "upps " << opt_res << '\n';
    }

    if (!sock.valid()) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    auto port_short = static_cast<int16_t>(std::strtol(port.data(), nullptr, 10));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_short);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if (bind(sock.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (sock.valid() && listen(sock.get(), SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
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
            fprintf(stderr, "Could not connect\n");
            exit(EXIT_FAILURE);
        }
    }

    return client_socket;
}


}
}
// export to om_tools
using descriptors::descriptor_base;
using descriptors::socket_fd;
using descriptors::file_descriptor;
}
