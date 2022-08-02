/**
 * examples for using simple_libpq_helper, wrapper around libpq, the postgres C api.
 *
*/

#include <libpq.hpp>
#include <boost/lexical_cast.hpp>
#include <connection_pool.hpp>

namespace ot = om_tools::libpq_helper;
using namespace om_tools::connection_pool;
//namespace op = om_tools::libpq_helper;

typedef Pool<ot::PGConnection> pg_pool;

void create_a_table() {
    auto conn = pg_pool::get_entry();

    boost::string_view stmt = "create table c ("
                              "id integer not null unique,"
                              "c char(8),"
                              "i integer,"
                              "cc char(20),"
                              "insert_ts TIMESTAMP (2) NOT NULL DEFAULT CURRENT_TIMESTAMP(2)"
                              ")";
    ot::Scoped_result result;
    result = conn->exec(stmt);
    if (!result) {
        std::cout << "Failed to create table\n";
        return;
    }
}

void insert_this(int32_t id, const char* s, int32_t i, const char* ss) {
    std::string stmt("insert into c values($1, $2, $3, $4);");

    ot::Params params;
    params.add(id).add(s).add(i).add(ss);

    ot::Scoped_result result;
    result = pg_pool::get_entry()->exec(stmt, params);

    if (!result) {
        std::cout << "insert failed\n";
    }
}

void create_data() {
    create_a_table();
    insert_this(1, "1", 11, "Some string thing 11");
    insert_this(2, "2", 22, "Some string thing 22");
    insert_this(3, "3", 33, "Some string thing 33");
    insert_this(4, "4", 33, "Some string thing 44");
    insert_this(5, "5", 33, "Some string thing 55");
    insert_this(6, "6", 33, "Some string thing 66");
    insert_this(7, "7", 33, "Some string thing 77");
}

void pass_params() {
    std::string stmt = "select i, c, id, cc from c where id = $1;";
    ot::Params params;
    params.add(2);
    ot::Scoped_result result(pg_pool::get_entry()->exec(stmt, params));

    if (result) {
        // get what ever type from the value
        // you can get a string from int columns but ints from
        // text columns may result in failed conversion.
        int32_t i = result.get_value<int32_t>(0, 2);
        std::string s = result.get_value<std::string>(0, 2);

        std::string i_s = result.get_value<std::string>(0, 0);
        std::string c = result.get_value<std::string>(0, 1);
        std::string cc = result.get_value<std::string>(0, 3);

        std::cout << s << '|' << i << ' ' << i_s << ' ' << c << ' ' << cc << '\n';
    }

    stmt = "select id, cc from c where c = $1";
    params.clear().add("3");
    result = pg_pool::get_entry()->exec(stmt, params);
    if (result) {
        std::cout << result.get_value<int>(0,0) << ' ' << result.get_value<std::string>(0,1)<< '\n';
    }
}

void print_all() {
    std::string stmt = "select * from c";
    ot::Scoped_result result(pg_pool::get_entry()->exec(stmt));
    result.list_tuples();
}

void count() {
    std::string stmt = "select count(*), max(i) from c";
    ot::Scoped_result result(pg_pool::get_entry()->exec(stmt));
    if (result) {
        ot::Scoped_result::row_iterator iter = result.begin();
        std::cout << iter.get<int>(0) << '|' << iter.get<int>(1) <<  '\n';
    }
}

bool has_data() {
    const char stmt[] = "SELECT EXISTS ("
                        "   SELECT FROM information_schema.tables"
                        "   WHERE table_name = 'c')";
    ot::Scoped_result result(pg_pool::get_entry()->exec(stmt));

    if (!result) {
        return false;
    } else {
        char exists = result.get_value<char>(0, 0);
        return exists == 't';
    }
}

void timestamps() {
    ot::Scoped_result result(pg_pool::get_entry()->exec("select extract (epoch from insert_ts) from c"));
    if (result) {
        ot::Timestamp timestamp = result.get_value<ot::Timestamp>(0,0);
        std::cout << timestamp << ' '
                << timestamp.to_time_t() << ' '
                << timestamp.to_timestamp_string()
                << '\n';
    }
    std::cout << "done\n";
}

[[maybe_unused]] void varchars() {
    ot::Scoped_result result(pg_pool::get_entry()->exec("select * from c"));
    if (result) {
        std::string v = result.get_value<std::string>(0, 0);
        std::string c = result.get_value<std::string>(0, 1);
        std::cout << v << ' ' << c << "\n";
    }
    std::cout << "\n";
}
int main() {
    try {
        if (!has_data()) {
            create_data();
        }
//        varchars(); todo create a table for this test
        timestamps();
        pass_params();
        print_all();
        count();
        if (pg_pool::get_entry()->test_connection()) {
            std::cout << "connection is fine\n";
        }
    } catch (const std::exception& e) {
        std::clog << "Upps " << e.what() << '\n';
    }
}
