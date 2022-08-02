
#include <gtest/gtest.h>
#include <boost/utility/string_view.hpp>
#include <connection_pool.hpp>
#define USE_GENERIC_POOL 0
#include <redis.hpp>

namespace ot = om_tools;
using namespace ot::connection_pool;

typedef Pool<ot::Redis> redis_pool;

TEST(redis_test, command) {
    auto redis = redis_pool::get_entry();
    ot::redis_error err = REDIS_OK;
    EXPECT_NO_THROW(redis->command("SET TEST_KEY TEST_VALUE"));
    EXPECT_TRUE(redis->good());
    EXPECT_STREQ(redis->const_char(), "OK");
    EXPECT_NO_THROW(redis->command("SET ANOTHER_TEST_KEY TEST_VALUE", err));
    EXPECT_TRUE(redis->good());
    EXPECT_STREQ(redis->const_char(), "OK");

    EXPECT_STREQ(redis->command("SET ANOTHER_TEST_KEY TEST_VALUE", err).const_char(), "OK");
    EXPECT_TRUE(redis->good());
    EXPECT_STREQ(redis->const_char(), "OK");

    EXPECT_THROW(redis->command("SET TEST_KEY"), ot::Redis_error);
    EXPECT_NO_THROW(redis->command("SET TEST_KEY", err));
    EXPECT_NE(err, REDIS_OK);
}

TEST(redis_test, command_fmt_format) {
    auto redis = redis_pool::get_entry();
    ot::redis_error err = REDIS_OK;

    EXPECT_NO_THROW(redis->command(fmt::format("HSET FMT_HASH_KEY val1 {} val2 {} val3 {}", 12, 13, 14), err));
    EXPECT_EQ(err, REDIS_OK);
    EXPECT_NO_THROW(redis->command(fmt::format("SET FMT_KEY {}", 13)));
    EXPECT_EQ(err, REDIS_OK);
}

TEST(redis_test, command_c_format) {
    auto redis = redis_pool::get_entry();
    ot::redis_error err = REDIS_OK;

    EXPECT_NO_THROW(redis->command_format(err, "HSET process:%s value %s", "TEST_KEY", "Some arbitrary string"));
    EXPECT_EQ(err, REDIS_OK);

    // test %b - binary safe string, needs length
    boost::string_view msg("Some arbitrary string");
    EXPECT_NO_THROW(redis->command_format(err, "HSET system:%s value %b", "TEST_KEY", msg.data(), msg.size()));
    EXPECT_EQ(err, REDIS_OK);
    EXPECT_NE(err, REDIS_ERR_OOM); // this is the error if %b didn't get a length

    boost::string_view val1("Some arbitrary string");
    boost::string_view val2("Another one");
    EXPECT_NO_THROW(redis->command_format(err, "HSET process:%s value %b val1 %b", "TEST_KEY", val1.data(),
                                          val1.size(), val2.data(), val2.size()));
    EXPECT_EQ(err, REDIS_OK);

}

TEST(redis_test, validate_connection) {
    auto redis = redis_pool::get_entry();
    ot::redis_error err = REDIS_OK;
    // check if the connections is "clean" i.e. no old response hanging around, the pool should reset before returning
    EXPECT_FALSE(redis->good());
    redis->set("valid", "true", err);
    EXPECT_TRUE(redis->good());
    redis->get("Non-existing-key");
    EXPECT_TRUE(redis->good());
    redis->get("valid");
    EXPECT_TRUE(redis->good());
    redis->get("");
    EXPECT_TRUE(redis->good());

    std::cout << '\n';
}

TEST(redis_test, rows_affected) {
    auto redis = redis_pool::get_entry();
    ot::redis_error err = REDIS_OK;

    // IDs are changed to end with and X, so the test case doesn't mess up if run on PROD/TEST/DEV server

    // first ensure example is not in redis already, or the rows affected are not as expected
    redis->remove("article:6019859X", err);
    redis->remove("article:5009799X", err);

    EXPECT_NO_THROW(redis->command_format(err, "HSET article:%s long_name %s short_name %s", "6019859X", "Red socks with onion pattern", "RED SOCKS"));
    EXPECT_EQ(redis->rows_affected(), 2); // created hash set with 2 values
    EXPECT_NO_THROW(redis->command_format(err, "HSET article:%s long_name %s short_name %s tech_name %s", "6019859X", "Red socks with onion pattern", "RED SOCKS", "RED SOCKS WITH ONION PATTERN"));
    EXPECT_EQ(redis->rows_affected(), 1); // updated the hash set with 1 more value, the values that doesn't change can be omitted
    EXPECT_NO_THROW(redis->command_format(err, "HSET article:%s range_name %s", "6019859X", "RED"));
    EXPECT_EQ(redis->rows_affected(), 1); // updated the hash set with 1 new value, values already in the hash may be omitted
    EXPECT_NO_THROW(redis->command_format(err, "HSET article:%s available %d", "5009799X", 56));
    EXPECT_EQ(redis->rows_affected(), 1); // updated hash set with 1 value
}

TEST(redis_test, hash) {
    auto redis = redis_pool::get_entry();
    if (!redis->good()) {

    }
    ot::redis_error err = REDIS_OK;

    EXPECT_NO_THROW(redis->hash_get("some:TEST_KEY", "value", err));
    EXPECT_NO_THROW(redis->hash_get("some:TEST _KEY", "", err));
    EXPECT_NO_THROW(redis->hash_get("som e:TEST_KEY", ""));
    EXPECT_STREQ(redis->const_char(), NULL); // expect not found
    EXPECT_NO_THROW(redis->hash_get("", ""));
    EXPECT_STREQ(redis->const_char(), NULL); // expect not found
}

