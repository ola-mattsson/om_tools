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

namespace om_tools {
namespace descriptors {

#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif

template<typename DESCRIPTOR_TYPE, DESCRIPTOR_TYPE INVALID = -1>
struct Descriptor_base {
    using descriptor_type = DESCRIPTOR_TYPE;
    static const descriptor_type invalid_socket = INVALID;
private:
    descriptor_type m_fd{invalid_socket};
public:
    Descriptor_base() = default;

    explicit Descriptor_base(descriptor_type fd) : m_fd(fd) {}

    // cannot be copied
    explicit Descriptor_base(const Descriptor_base &) = delete;

    // can be moved, the source will be invalidated
    Descriptor_base(Descriptor_base &&other) noexcept {
        m_fd = other.m_fd;
        other.m_fd = -1;
    }

    // close if valid
    ~Descriptor_base() { if (valid()) { close(); }}

    // cannot be copy assigned
    Descriptor_base &operator=(const Descriptor_base &other) = delete;

    // can be move-assigned, the source will be invalidated
    Descriptor_base &operator=(Descriptor_base &&other) noexcept {
        m_fd = other.m_fd;
        other.m_fd = -1;
        return *this;
    }

    constexpr void set(descriptor_type desc) {
        m_fd = desc;
    }
    [[nodiscard]]
    constexpr descriptor_type get() const {
        return m_fd;
    }

    constexpr void close() const {
        ::close(m_fd);
    }

    [[nodiscard]]
    constexpr bool valid() const { return m_fd != invalid_socket; }

    constexpr explicit operator descriptor_type() const { return m_fd; }

};

#if __cplusplus >= 201103L
}
#endif

}
// export to om_tools
using descriptors::Descriptor_base;
}
