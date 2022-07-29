#pragma once

/**
 * a socket wrapper, of sorts
 *
 * An instance of valid descriptor_case WILL attempt to close, when it goes out of scope
 *
 * The main point of this class is to guarantee that close(int fd) is called when it should.
 * "centralising" the closing can be achieved with goto labels but the housekeeping code
 * is still not as safe from early returns and C++ exceptions.
 *
 * about the inline namespace
 * https://youtu.be/rUESOjhvLw0
 */

#include <unistd.h>
#include <netdb.h>

namespace om_tools {
namespace descriptors {
inline namespace v1_0_0 {

template<typename DESCRIPTOR_TYPE, DESCRIPTOR_TYPE INVALID = -1>
struct descriptor_base {
    using descriptor_type = DESCRIPTOR_TYPE;
    static const descriptor_type invalid_socket = INVALID;
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

    constexpr void set(descriptor_type desc) {
        fd = desc;
    }
    [[nodiscard]]
    constexpr descriptor_type get() const {
        return fd;
    }

    constexpr void close() const {
        ::close(fd);
    }

    [[nodiscard]]
    constexpr bool valid() const { return fd != invalid_socket; }

    constexpr explicit operator descriptor_type() const { return fd; }

};

}
}
// export to om_tools
using descriptors::descriptor_base;
}
