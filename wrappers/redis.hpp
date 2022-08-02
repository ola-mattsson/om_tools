#pragma once

/**
 * Simple to use C++ interface to hiredis.
 *
 *
 * Error handling
 * the redisContext has an err field, 0 == last command was successful else
 * error messages is in the errstr field.
 *
 * the redisReply type may be REDIS_REPLY_ERROR, then the str field is the error message.
 * the str field contains status of command or the string if type is REDIS_REPLY_STRING
 */


#include <hiredis/hiredis.h>
#include <vector>
#include <fmt/core.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <chrono>

namespace om_tools {

    typedef int32_t redis_error;

    class Redis_error : public std::runtime_error {
        using string_view = std::string_view;

    public:
        explicit Redis_error(string_view msg) : std::runtime_error(msg.data()) {}
    };

    class Redis {

        // manages all operations on the hiredis api
        class Redis_context {
            bool m_owns_context;

            redisContext &m_context;

        public:
            Redis_context(const Redis_context &) = delete;

            Redis_context(Redis_context &&other) noexcept:
                    m_owns_context(other.m_owns_context),
                    m_context(other.m_context) { other.m_owns_context = false; }

            explicit Redis_context(redisContext *context) :
                    m_owns_context(false),
                    m_context(*context) {}

            explicit Redis_context(bool TCP = true, std::string_view host = "127.0.0.1", int32_t port = 6379) :
                    m_owns_context(true),
                    m_context(TCP ? *redisConnect(host.data(), port) : *redisConnectUnix(host.data())) {}

//            constexpr
            ~Redis_context() {
//                puts("Redis_context dtor");
                if (m_owns_context) {
                    redisFree(&m_context);
                }
            }

//            constexpr
            Redis_context &operator=(Redis_context &&other) noexcept {
                m_context = other.m_context;
                other.m_owns_context = false;
                return *this;
            }

            /**
             * Basic commands like "SET KEY 1"
             * @param cmd the command
             * @return the redisReply
             */
            redisReply *exec(std::string_view cmd) noexcept {
                void *void_result = redisCommand(&m_context, cmd.data());
                redisReply *result = static_cast<redisReply *>(void_result);
                return result;
            }

            /**
             * Complex commands, and strings with spaces requires
             * special handling
             * @param fmt format string, %s regular strings, %b binary strings requires length
             * @param args va_list
             * @return the redisReply
             */
            redisReply *exec(const char *fmt, va_list *args) noexcept {
                void *void_result = redisvCommand(&m_context, fmt, *args);
                redisReply *result = static_cast<redisReply *>(void_result);
                return result;
            }

            constexpr redisContext &get_context() { return m_context; };

#if __cplusplus >= 201703

            [[nodiscard]]
#endif
            constexpr bool good() const noexcept { return m_context.err == REDIS_OK; }

#if __cplusplus >= 201703

            [[maybe_unused]] [[nodiscard]]
#endif
            constexpr redis_error error() const noexcept { return m_context.err; }

#if __cplusplus >= 201703

            [[nodiscard]]
#endif
            constexpr const char *error_str() const noexcept { return m_context.errstr; }
        };

        // A bit like a transaction, kind of, it queues commands
        // and executes them when EXEC is called, or discards then if
        // DISCARD is called instead
        struct Txn {
            Redis_context &m_ctx;
            bool committed;

            explicit Txn(Redis_context &context) : m_ctx(context), committed(false) {
                m_ctx.exec("MULTI");
            }

            void commit() noexcept {
                if (m_ctx.good()) {
                    m_ctx.exec("EXEC");
                    committed = true;
                }
            }

            ~Txn() noexcept { if (!committed) { m_ctx.exec("DISCARD"); }}
        };

        Redis_context m_ctx;

        redisReply *m_reply;

//        std::chrono::time_point<std::chrono::system_clock> created{};
//    bool in_use{false};

        [[nodiscard]]
        constexpr redisReply *raw_reply() const noexcept { return m_reply; }

        // so we don't need to null check the reply everywhere
        constexpr void free_reply() noexcept {
            if (m_reply) {
                freeReplyObject(m_reply);
                m_reply = nullptr;
            }
        }

    public:
        enum CONN_TYPE {
            C_TCP,
            C_UDS
        };

        Redis(const Redis &) = delete; // don't copy

        Redis(Redis &&other) noexcept: m_ctx(std::move(other.m_ctx)), m_reply(other.m_reply) {}

//        constexpr
        Redis &operator=(Redis &&other) noexcept {
            std::swap(m_ctx, other.m_ctx);
            std::swap(m_reply, other.m_reply);
            other.m_reply = nullptr; // make sure 'other' doesn't free the reply
            return *this;
        }

        /**
         * Create a Redis object that owns it's redisContext
         * This should suffice for most use cases
         * @param type Connection type CONN_TYPE c_tcp or c_uds
         * @param host tcp host or uds path
         * @param port if CONN_TYPE c_tcp
         */
        explicit Redis(CONN_TYPE type = C_TCP, std::string_view host = "127.0.0.1", int32_t port = 6379) :
                m_ctx(type == C_TCP, host.data(), port), m_reply(nullptr) {
        }

        /**
         * Create a Redis object with a pointer to an existing redisContext.
         * It's not my redisContext so I will not release it
         * @param context pointer
         */
        explicit Redis(redisContext *context) : m_ctx(context), m_reply(nullptr) {}

        /**
         * Create a Redis object with a reference to an existing redisContext.
         * It's not my redisContext so I will not release it
         * @param context reference
         */
        explicit Redis(redisContext &context) : m_ctx(&context), m_reply(nullptr) {}

//        constexpr
        ~Redis() noexcept {
//            puts("Redis dtor");
            free_reply();
        }

#if __cplusplus >= 201703

        [[maybe_unused]]
#endif
        constexpr redisContext &get_context() { return m_ctx.get_context(); }

        // throws Redis_error
        Redis &multi_command(const std::vector<std::string_view> &cmds) {
            redis_error err = REDIS_OK;
            multi_command(cmds, err);
            if (err != REDIS_OK) {
                throw Redis_error(m_ctx.error_str());
            }
            return *this;
        }

        // does not throw, redis error code in err
        Redis &multi_command(const std::vector<std::string_view> &cmds, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;
            Txn txn(m_ctx);

            std::vector<std::string_view>::const_iterator iter = cmds.begin();
            for (; iter != cmds.end(); ++iter) {
                m_ctx.exec(*iter);
                if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                    err = m_ctx.error();
                }
            }
            txn.commit();

            return *this;

        }

        // TODO SET can return the previous value if updating. this function should perhaps do that too.
        template<typename T>
//        constexpr
        Redis &set(std::string_view key, const T &value, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;
            std::string expr = "SET ";
            expr += key;
            expr += ' ';
            expr += value;
            command(expr, err);
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        Redis &hash_get(std::string_view key, std::string_view value, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;

            command_format(err, "HGET %s %s", key.data(), value.data());
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        Redis &hash_get(std::string_view key, std::string_view value) {
            redis_error err = REDIS_OK;

            hash_get(key, value, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("{} not found", key));
            }
            return *this;
        }

        /**
         * Set a vector of key/values to a hash
         * @param key hashkey
         * @param field_names
         * @param values
         * @return
         */
        Redis &hash_set(std::string_view key, const std::vector<std::string> &field_names,
                                  const std::vector<std::string> &values) {
            redis_error err;
            hash_set(key, field_names, values, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("{} hash_set failed", key));
            }
            return *this;
        }

        Redis &hash_set(std::string_view key, const std::vector<std::string> &field_names,
                                  const std::vector<std::string> &values, redis_error &err) {
            BOOST_ASSERT_MSG(field_names.size() == values.size(), "Different number of names and values");
            std::string expr("HSET ");
            expr += key;
            std::vector<std::string>::const_iterator field = field_names.begin();
            std::vector<std::string>::const_iterator value = values.begin();
            for (; field_names.end() != field; ++field, ++value) {
                expr += ' ' + *field = ' ';
                expr += *value;
            }

            command(expr);
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        /**
         * Set a vector of key/values, stored in key_values to hash
         * @param key
         * @param key_values
         * @return
         */
        Redis &hash_set(std::string_view key, const std::vector<std::string> &key_values) {
            redis_error err;
            hash_set(key, key_values, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("hash_set failed with {}", err));
            }
            return *this;
        }

        Redis &hash_set(std::string_view key, const std::vector<std::string> &key_values, redis_error &err) {
            std::string expr("HSET ");
            expr += key;
            std::vector<std::string>::const_iterator key_value = key_values.begin();
            for (; key_values.end() != key_value; ++key_value) {
                expr += ' ' + *key_value;
            }
            command(expr);
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }

            return *this;
        }

        /**
         * Set a key/value to a hash
         * @param key       Key to create/update
         * @param field     Field to set
         * @param value     Value of the field
         * @return *this
         */
        Redis &hash_set(std::string_view key, std::string_view field, std::string_view value) {
            redis_error err;
            hash_set(key, field, value, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("{} hash_set failed", key));
            }
            return *this;
        }

        Redis &
        hash_set(std::string_view key, std::string_view field, std::string_view value, redis_error &err) {
            command_format(err, "HSET %s %s %b", key.data(), field.data(), value.data(), value.size());
            err = REDIS_OK;
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        Redis &get(std::string_view key) {
            redis_error err = REDIS_OK;
            get(key, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("{} not found ", key));
            }
            return *this;
        }

        Redis &get(std::string_view key, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;
            command_format(err, "GET %s", key.data());
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        Redis &remove(std::string_view key) {
            redis_error err = REDIS_OK;
            remove(key, err);
            if (err != REDIS_OK) {
                throw Redis_error(fmt::format("{} failed remove", key));
            }
            return *this;

        }

        Redis &remove(std::string_view key, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;
            command_format(err, "DEL %s", key.data());
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = m_ctx.error();
            }
            return *this;
        }

        Redis &command(std::string_view cmd) {
            redis_error err = REDIS_OK;
            command(cmd, err);
            if (err == REDIS_REPLY_ERROR) {
                throw Redis_error(fmt::format("{} {}", cmd, (m_reply ? m_reply->str : " Failed")));
            }
            return *this;
        }

        Redis &command(std::string_view cmd, redis_error &err) noexcept {
            free_reply();
            err = REDIS_OK;
            m_reply = m_ctx.exec(cmd);
            if (m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR) {
                err = REDIS_REPLY_ERROR;
            }
            return *this;
        }

        Redis &command_format(redis_error &err, const char *fmt, ...) noexcept {
            free_reply();
            err = REDIS_OK;
            va_list args = {};
            va_start(args, fmt);
            m_reply = m_ctx.exec(fmt, &args);
            va_end(args);
            if (!good()) {
                err = m_ctx.error();
            }
            return *this;
        }

        constexpr int32_t rows_affected() {
            if (m_reply->type == REDIS_REPLY_INTEGER) {
                // the redis->integer is long long, > 2 billions rows is probably enough
                // still, so sorry about the cast, saves callers doing it.
                return static_cast<int32_t>(m_reply->integer);
            }
            return 0;
        }

        Redis &out(std::ostream &ostream, bool newline = false) noexcept {
            ostream << str() << (newline ? "\n" : "");
            return *this;
        }

        /**
         * Get the string value.
         * Note that this is only interesting after for example "GET some_key", then this is the value
         * if the key exists or NULL otherwise.
         * After for example "SET some:key some_value", str is "OK", after the same command
         * while in "MULTI" mode str is "QUEUED".
         * If the command failed, like "HSET some:key something", str is an message claiming the command
         * was invalid. In this case the command should have been "HSET some:key something some_value"
         * @return a const char*, null if not found
         */
        [[nodiscard]]
        constexpr const char *const_char() const noexcept {
            return m_reply ? m_reply->str : NULL;
        }

        [[nodiscard]]
        constexpr std::string_view str() const noexcept {
            return m_reply && m_reply->str ? m_reply->str : "";
        }

        /**
         * Get a value converted. redisReply to GET is string
         * @tparam T
         * @return
         */
        template<typename T>
//        constexpr
        T get_val() const noexcept {
            T result = T();
            if (good()) {
                if (m_reply->type == REDIS_REPLY_STRING) {
                    try {
                        result = boost::lexical_cast<T>(str());
                    } catch (const std::bad_cast &e) {
                        std::clog << "Lexical cast " << m_reply->str << " Failed: " << e.what() << '\n';
                    }
                }
            }
            return result;
        }


        /**
         * Check last command.
         * @return true if last command was successful
         */
        [[nodiscard]]
//        constexpr
        bool good() const noexcept {
            // the context can hold an error, this is IO, out of memory and such
            // a command that ret
            if (m_ctx.good()) {
                // A command can fail and still be "successful", when calling for example "GET key"
                // when the key doesn't exist, the type is REDIS_REPLY_NIL and reply.str is nil.
                return m_reply != nullptr;
            }
            return false;
        }

        /**
         * Check connection.
         * @return true if connection is good
         */
        [[nodiscard]]
        constexpr
        bool good_connection() const noexcept {
            return m_ctx.good();
        }

        /**
         * Drop any old response
         */
        void reset() noexcept {
            free_reply();
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept { return m_reply != nullptr; }

    };

    ////////////////////////////////////////////////////////////////////////////
    // Redis connection pool
#if USE_GENERIC_POOL
    template<class POOL>
    struct Pool_entry {
        Redis *m_pooled;
        Pool_entry() : m_pooled(new Redis) {}
        Pool_entry(Redis* pooled) : m_pooled(pooled) {}
        Pool_entry(const Pool_entry&) = delete;
        ~Pool_entry() {
            if (m_pooled->good()) {
                // Put the pooled connection back into the pool vector.
                POOL::return_entry(m_pooled);
            } else {
                // The Redis object is good only if it was used, successfully,
                // so if you instansiate one without using it it will be deleted.
                delete m_pooled;
            }
        }

        Redis* operator->() {
            return m_pooled;
        }
        Redis& get() {
            return *m_pooled;
        }
    };

    class Redis_pool {

        static void return_entry(Redis* entry) {
            g_redis_pool.push_back(entry);
        }

    public:

        static std::vector<Redis*> g_redis_pool;

        friend struct Pool_entry<Redis_pool>;

        typedef Pool_entry<Redis_pool> entry_type;

        static entry_type get_entry() {
            if (g_redis_pool.empty()) {
                return entry_type();
            } else {
                auto front = g_redis_pool.front();
                g_redis_pool.erase(g_redis_pool.begin());
                return entry_type(front);
            }
        }
    };
#endif
}
