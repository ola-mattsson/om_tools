#pragma once

/**
 * A simple generic connection pool
 *
 * The Pool_entry is used as a stack object and will put it self back
 * into the pool on destruction.
 * the pooled connection is accessed with the arrow operator, or
 * get() in case the arrow operator looks corny to you.
 *
 * The pooled connection class must implement `bool your_class::good_connection() const` and
 * 'void reset()', this way this pool can choose not to save a bad connection and to
 * supply an instance without any old response
 *
 * Note: don't hang on to the pointer returned by get(), if the
 * Pool_entry object goes out of scope, you have a dangling pointer and
 * never forget:
 *      "being careful doesn't scale" /Bjarne Stroustrup
 *
 * Usage Example:
 *
 * auto connection = Pool<Type_of_connection>::get_entry();
 * // call some method with the arrow operator
 * connection->some_method();
 * // or with get()
 * connection.get().some_method();
 *
 * To use only this header, define the pools list with
 *
 * template<class CONNECTION>
 * std::vector<CONNECTION*> Pool<CONNECTION>::g_pool;
 *
 * Improvement opportunities:
 * 1. aging, drop unused connections after a period like an hour
 * 2.
*/

#include <vector>
namespace om_tools::connection_pool {
#if __cplusplus >= 201103L
inline namespace v1_0_0 {
#endif

template<class POOLED, class POOL>
class Pool_entry {
    POOLED *m_pooled;
public:

    Pool_entry() : m_pooled(new POOLED) {}

    explicit Pool_entry(POOLED *pooled) : m_pooled(pooled) {}

    Pool_entry(const Pool_entry &) = delete;

    ~Pool_entry() {
        if (m_pooled->good_connection()) {
            m_pooled->reset();
            POOL::return_entry(m_pooled);
        } else {
            delete m_pooled;
        }
    }

    POOLED *operator->() const {
        return m_pooled;
    }

    POOLED &get() const {
        return *m_pooled;
    }
};

/**
*
* Get connection with Pool<Type>::get_entry(), this will create a connection and return it but NOT pool it
* Return the connection with Pool<Type>::return_entry
*/

template<class CONNECTION>
class Pool {
    static void return_entry(CONNECTION *entry) {
        g_pool.push_back(entry);
    }

public:
    typedef std::vector<CONNECTION *> pool_vector;
    static pool_vector g_pool;
    typedef Pool_entry<CONNECTION, Pool<CONNECTION>> entry_type;

    friend class Pool_entry<CONNECTION, Pool<CONNECTION>>;

    static entry_type get_entry() {
        if (g_pool.empty()) {
            return entry_type();
        } else {
            auto front = g_pool.front();
            g_pool.erase(g_pool.begin());
            return entry_type(front);
        }
    }
};

template<class CONNECTION>
std::vector<CONNECTION *> Pool<CONNECTION>::g_pool;

#if __cplusplus >= 201103L
}
#endif

}

