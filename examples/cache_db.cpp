#include <boost/algorithm/string/trim.hpp>
#include <libpq.hpp>
#include <redis.hpp>

using namespace om_tools;
class Redis_util {
    Redis redis;
public:
    void load_articles() {
        libpq_helper::PGConnection pg("host=localhost user=pgmhs dbname=mhs2");
        pg.exec("set search_path to iwardb_958");

        std::string_view get_arts_stmt =
            "select trim(art), trim(ldes_unicode), trim(sdes_unicode), trim(range_name_unicode) from iwaab";

        libpq_helper::Scoped_result result(pg.exec(get_arts_stmt.data()));

        redis_error err;
        redis.command("MULTI", err);
        for (auto item = result.begin(); item != result.end(); ++item) {
            std::string id = item.get<std::string>(0);
            std::string name = item.get<std::string>(1);
            std::string sdes = item.get<std::string>(2);
            std::string range_name = item.get<std::string>(3);

            redis.command_format(err,
                                 "HSET article:%b ldes %b sdes %b range_name %b ll %ld dub %.2f",
                                 id.c_str(), id.size(),
                                 name.c_str(), name.size(),
                                 sdes.c_str(), sdes.size(),
                                 range_name.c_str(), range_name.size(),
                                 100LL, 567.89
            );
        }
        redis.command("EXEC");
    }

    std::string art_name(std::string_view art) {
        redis_error err = REDIS_OK;
        redis.command_format(err, "HGET article:%s ldes", art.data());
        if (err == REDIS_OK) {
            return redis.str().data();
        }
        return "";
    }
    int32_t art_int(std::string_view art) {
        redis_error err = REDIS_OK;
        redis.command_format(err, "HGET article:%s ll", art.data());
        if (err == REDIS_OK) {
            return redis.get_val<int32_t>();
        }
        return 0;
    }
    double_t art_double(std::string_view art) {
        redis_error err = REDIS_OK;
        redis.command_format(err, "HGET article:%s dub", art.data());
        if (err == REDIS_OK) {
            return redis.get_val<double>();
        }
        return 0;
    }
};


int main() {
    Redis r;
    redis_error err = REDIS_OK;
    if (r.set("test", 786.87, err).good()) {
        double val = r.get("test", err).get_val<double>();
        std::cout << val << '\n';
        if (r.get("test").good()) {
            std::cout << "Successfully retrieved key 'test': " << r.get_val<std::string>() << '\n';
        }
        if (r.remove("test", err).good()) {
            std::cout << "successfully removed key 'test': " << r.rows_affected() << " row deleted\n";
        }
        if (r.remove("test", err).good()) {
            std::cout << "successfully removed key 'test': " << r.rows_affected() << " row deleted\n";
        }
    }
    if (r.get("test").good()) {
        std::cout << "get non-existent key successfully\n";
        if (r.get_val<std::string>().empty()) {
            std::cout << "get_val non-existent key empty as expected" << '\n';
        }
        if (r.const_char() == nullptr) {
            std::cout << "const_char non-existent key NULL as expected" << '\n';
        }
    }

//    Redis_util redis;
//    redis.load_articles();
//    std::cout << "lets go" << '\n';
//    std::cout
//        << "article 70490948: " << redis.art_name("20464621") << ' ' << redis.art_int("20464621") << ' ' << redis.art_double("20464621") << '\n'
//        << "article 70490948: " << redis.art_name("10454463") << ' ' << redis.art_int("10454463") << ' ' << redis.art_double("10454463") << '\n'
//        << "article 70490948: " << redis.art_name("80488092") << ' ' << redis.art_int("80488092") << ' ' << redis.art_double("80488092") << '\n'
//        << "article 70490948: " << redis.art_name("20456541") << ' ' << redis.art_int("20456541") << ' ' << redis.art_double("20456541") << '\n'
//        << "article 70490948: " << redis.art_name("00498486") << ' ' << redis.art_int("00498486") << ' ' << redis.art_double("00498486") << '\n'
//        << "article 70490948: " << redis.art_name("10456570") << ' ' << redis.art_int("10456570") << ' ' << redis.art_double("10456570") << '\n'
//        << "article 70490948: " << redis.art_name("30442898") << ' ' << redis.art_int("30442898") << ' ' << redis.art_double("30442898") << '\n'
//        << "article 70490948: " << redis.art_name("20462995") << ' ' << redis.art_int("20462995") << ' ' << redis.art_double("20462995") << '\n'
//        << "article 70490948: " << redis.art_name("80488087") << ' ' << redis.art_int("80488087") << ' ' << redis.art_double("80488087") << '\n'
//        << "article 50097995: " << redis.art_name("20464697") << ' ' << redis.art_int("20464697") << ' ' << redis.art_double("20464697") << '\n';
}
