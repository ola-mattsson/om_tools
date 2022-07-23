#pragma once

/**
 * a socket wrapper, of sorts
 *
 * An instance of socket_fd WILL close when it goes out of scope
 *
 * about the inline namespace
 * https://youtu.be/rUESOjhvLw0
 */

#if __cplusplus <= 201103L
// TODO make less standard reliant
#error C++11 and above only
#endif
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
protected:
    void set(descriptor_type desc) {
        fd = desc;
    }
public:
    [[nodiscard]]
    descriptor_type get() const {
        return fd;
    }
    descriptor_base() = default;

    explicit descriptor_base(descriptor_type fd_) : fd(fd_) {}

    explicit descriptor_base(const descriptor_base &) = delete;

    descriptor_base &operator=(const descriptor_base &other) = delete;

    // move constructor
    descriptor_base(descriptor_base &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
    }

    // move assignment
    descriptor_base &operator=(descriptor_base &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
        return *this;
    }

    ~descriptor_base() { if (valid()) { close_socket(); }}

    void close_socket() const {
        std::cout << "Closing socket\n";
        close(fd);
    }

    [[nodiscard]]
    bool valid() const { return fd != invalid_socket; }

    explicit operator descriptor_type() const { return fd; }


};

class socket_fd : public descriptor_base {

public:
    socket_fd() : descriptor_base() {}
    explicit socket_fd(descriptor_type fd_) : descriptor_base(fd_) {}

    void wait_request(const socket_fd &server_socket);

    [[nodiscard]]
    socket_fd wait_request() const;

    static socket_fd create_server_socket(int32_t port);
    static socket_fd create_client_socket(const char* host, int32_t port);
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

inline socket_fd socket_fd::create_server_socket(int32_t port) {

    sockaddr_in addr = {};

    socket_fd sock(socket(AF_INET, SOCK_STREAM, 0));
    int opt_val = 1;
    if (int opt_res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val); opt_res != 0) {
        std::cout << "upps " << opt_res << '\n';
    }

    if (!sock.valid()) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
//    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock.get(), reinterpret_cast<sockaddr *>(&addr), sizeof addr) == -1) {
        perror("bind");
    }

    if (sock.valid() && listen(sock.get(), SOMAXCONN) == -1) {
        perror("listen");
    }

    return sock;
}

socket_fd socket_fd::create_client_socket(const char *host, const char* port) {

    addrinfo hints = {};

    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_NUMERICHOST;
    hints.ai_protocol = 0;

    addrinfo *result = nullptr;
    if (int s = getaddrinfo(host, port, &hints, &result); s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    socket_fd client_socket;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
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

    freeaddrinfo(result);
    return client_socket;
}


}
}
// export to om_tools
using descriptors::descriptor_base;
using descriptors::socket_fd;
}
