#pragma once

/**
 * a socket wrapper, of sorts
 *
 * An instance of socket_fd WILL close when it goes out of scope
 *
 * about the inline namespace
 * https://youtu.be/rUESOjhvLw0
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>


class socket_fd {
    using socket_type = int;
    static const socket_type invalid_socket = -1;

    socket_type fd{invalid_socket};
public:
    socket_fd() = default;

    explicit socket_fd(socket_type fd_) : fd(fd_) {}

    explicit socket_fd(const socket_fd &) = delete;

    socket_fd &operator=(const socket_fd &other) = delete;

    // but it can be moved
    socket_fd(socket_fd &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
    }

    // move assignment
    socket_fd &operator=(socket_fd &&other) noexcept {
        other.fd = -1;
        return *this;
    }

    ~socket_fd() { if (valid()) { close_socket(); }}

    void close_socket() const {
        close(fd);
    }

    [[nodiscard]]
    bool valid() const { return fd != invalid_socket; }

    explicit operator socket_type() const { return fd; }

    [[nodiscard]]
    socket_type get() const { return fd; }

    static socket_fd create_socket(int32_t port);

    void wait_request(const socket_fd &server_socket);

    socket_fd wait_request() const;
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

    fd = accept(server_socket.get(), reinterpret_cast<sockaddr *>(&addr), &addrlen);

    if (fd == invalid_socket) {
        perror("accept");
    }
}

inline socket_fd socket_fd::create_socket(int32_t port) {

    sockaddr_in addr = {};

    socket_fd sock(socket(AF_INET, SOCK_STREAM, 0));

    if (!sock.valid()) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
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
