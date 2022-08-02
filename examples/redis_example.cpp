#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <redis.hpp>
#include <connection_pool.hpp>

using namespace om_tools;
using namespace connection_pool;
Pool<Redis>::entry_type get_redis() {
    return  Pool<Redis>::get_entry();
}

typedef Pool<Redis> Redis_pool;

void connection_examples() {
    // How to do it

    // This will connect to defaults localhost: 127.0.0.1:6379 (default redis port)
    auto the_redis_connection = Redis_pool::get_entry();
    if (!the_redis_connection->good()) {
        std::cout << "can't connect to redis over default host:port\n";
        return;
    }

    // another connection with specified type, ip address and port
    Redis a_specified_redis_tcp_connection(Redis::C_TCP, "127.0.0.1", 6379);
    if (!a_specified_redis_tcp_connection.good()) {
        std::cout << "can't connect to redis over tcp localhost:6379\n";
    }

    // Unix Domain Socket connection with specified type and UDS path
    // not tested yet!
    // maybe this guide works https://guides.wp-bullet.com/how-to-configure-redis-to-use-unix-socket-speed-boost/
    Redis a_specified_redis_uds_connection(Redis::C_UDS, "/tmp/redis.sock");
    if (!a_specified_redis_uds_connection.good()) {
        std::cout << "can't connect to redis over uds\n";
    }

    // How NOT to do it even if you can:
    // manually created, don't forget to close it... YOU WILL FORGET, one or another way, maybe not today
    // maybe not you but somehow the connection will be forgotten, because "being careful doesn't scale"
    // so don't do this.
    redisContext *redis_conn = redisConnect("127.0.0.1", 6379);
    // pass your own redisContext to the redis class
    Redis non_owning_ptr(redis_conn);
    Redis non_owning_ref(*redis_conn);
    // release it
    redisFree(redis_conn);
}

void simple_set() {
    auto connection = get_redis();
    std::cout << connection->command("SET count 0").str() << '\n';
    std::cout << connection->command("GET count").str() << '\n';
    connection->command("INCR count")
        .command("INCR count")
        .command("INCR count")
        .command("GET count")
        .out(std::cout, true) // out put the last command to stdout
        .command("INCR count");
}

void multi_commands() {
    // a container of commands
    std::vector<std::string_view> commands;
    commands.emplace_back("SET counter 0");
    commands.emplace_back("INCR counter");
    commands.emplace_back("INCR counter");
    commands.emplace_back("INCR counter");

    auto connection = get_redis();

    // run them
    connection->multi_command(commands);

    // check the result
    std::cout << "counter: " << connection->command("GET counter").str() << '\n';
}

// just an example use. the redis action is in the store method
struct some_struct {
    std::string name;
    std::string job;

    some_struct(std::string in_name, std::string in_job)
        : name(std::move(in_name)), job(std::move(in_job)) {}

    bool store(Redis &connection) const {
        std::string cmd("hset S:" + name + " job " + job);
        return connection.command(cmd).good();
    }
};

void records() {
    auto connection = get_redis();

    // create an object and ask it to store itself in the provided redis connection
    some_struct("George", "computers").store(connection.get());
    some_struct("Ephraim", "Admin").store(connection.get());
    some_struct("Cesar", "Tyrant").store(connection.get());

    connection->command("HGETALL S:George");
    std::cout << connection->command("GET hej").str() << '\n';

    std::cout << '\n';
}

void prop_list() {
    typedef std::vector<std::pair<std::string, std::string> > pairs;
    pairs props;
    props.push_back(std::make_pair("name", "Shoe"));
    props.push_back(std::make_pair("smelly", "YES"));
    props.push_back(std::make_pair("comfy", "Sure"));

    std::stringstream stream;
    stream << "HSET item:1";
    for (auto & prop : props) {
        stream << ' ' << prop.first << ' ' << prop.second;
    }
    std::string cmd = stream.str();

    {
        auto conn = Pool<Redis>::get_entry();
        conn->command(cmd);

        if (conn->good()) {
            std::cout << "Hash Successfully stored!\n";
        } else {
            std::cout << "Failed to store hash: " << conn->str() << '\n';
        }

        conn->command("GET article:");
    }
}

void like_logical_name() {
    Redis redis;
    for (int i(0); i < 1; ++i) {
        redis.command("GET func_loglevel");
        int32_t loglevel = redis.get_val<int32_t>();
        std::cout << loglevel << '\n';
        sleep(1);
    }
}

void types() {
    Redis redis;
    redis_error err = REDIS_OK;
    redis.command("SET NUMBER 1", err);
    auto a = redis.command("GET NUMBER", err).get_val<int32_t>();
    redis.command("INCR NUMBER", err);
    a = redis.command("GET NUMBER", err).get_val<int32_t>();
    std::cout << a << '\n';
    redis.command("SET STR 1", err);
    std::cout << redis.command("GET STR", err).str() << '\n';
    std::cout << redis.command("GET NO_STR", err).str() << '\n';

}

std::string static_connection() {
    static Redis redis;
    redis_error err = REDIS_OK;
    const char *resp = redis.get("TEST", err).const_char();
    if (!redis.good()) {
        std::clog << "failed " << __FUNCTION__ << '\n';
    }
    return resp ? resp : "";
}

std::string new_connection() {
    Redis redis;
    redis_error err = REDIS_OK;
    if (redis.good()) {
        const char *resp = redis.get("TEST", err).const_char();
        if (!redis.good()) {
            std::clog << "failed " << __FUNCTION__ << '\n';
        }
        return resp ? resp : "";
    }
    return "";
}

std::string provided_connection(Redis &redis) {
    redis_error err = REDIS_OK;
    const char *resp = redis.get("TEST", err).const_char();
    if (!redis.good()) {
        std::clog << "failed " << __FUNCTION__ << '\n';
        return "";
    }
    return resp ? resp : "";
}

std::string pooled_connection() {
    auto redis = Pool<Redis>::get_entry();
    redis_error err = REDIS_OK;
    const char *resp = redis->get("TEST", err).const_char();
    if (!redis->good()) {
        std::clog << "failed " << __FUNCTION__ << '\n';
        return "";
    }
    return resp ? resp : "";
}

int main(int argc, const char **argv) {
    std::cout << __cplusplus << '\n';
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
    std::cout << "address sanitizer enabled";
#  endif
#endif


    {
        auto t = Redis_pool::get_entry();
    }
    {
        // create 3 connections
        [[maybe_unused]] auto r1 = Redis_pool::get_entry();
        [[maybe_unused]] auto r2 = Redis_pool::get_entry();
        [[maybe_unused]] auto r3 = Redis_pool::get_entry();

        // creating a connection, using it and dropping it, i.e. this scope
        // doesn't HOLD the 4:th connection, it is returned to the pool after the semicolon, but the
        // subsequent requests will return the same pooled connection.
        redis_error err;
        get_redis()->set("meck_43", "43", err);
     // ^^^^^^^^^    ^^^                      ^        get, use and drop

     // use the same connection again
        get_redis()->set("meck_44", "44", err);
    }

    {
        auto redis = Redis_pool::get_entry();
        redis_error err;
        // use the arrow operator
        redis->set("TEST", 54321, err);
        // use get()
        redis.get().set("TEST", 54321, err);
    }

    [[maybe_unused]] int64_t variant = (argc > 1 ? std::strtol(argv[1], nullptr, 10) : 0L);
    for (int i(0); i < 10000; ++i) {
        switch (variant) {
            case 1:
                // function has a static connection
                static_connection();
                break;
            case 2:
                // connection is made for every call
                new_connection();
                break;
            case 3: {
                // provide the connection
                static Redis redis;
                provided_connection(redis);
                break;
            }
            case 4:
                // use a pooled connection
                pooled_connection();
                break;
            default:
                std::cout << '.';
        }
    }
    // will print 4 unless I've changed something and forgot to update this.
    std::cout << "Redis pool entries: " << Redis_pool::g_pool.size() << '\n';


}
